#include "RadixTree.h"
#include "RdmaBuffer.h"

RadixTree::RadixTree(DSM *dsm) : dsm(dsm) {
  assert(dsm->is_register());
  root_ptr_ptr = get_root_ptr_ptr();

  auto page_buffer = (dsm->get_rbuf(0)).get_page_buffer();
  auto root_addr = dsm->alloc(kLeafPageSize);
  auto root_page = new (page_buffer) LeafNode;
  
  root_page->set_consistent();
  dsm->write_sync(page_buffer, root_addr, kLeafPageSize);

  auto cas_buffer = (dsm->get_rbuf(0)).get_cas_buffer();
  bool res = dsm->cas_sync(root_ptr_ptr, 0, root_addr.val, cas_buffer);

}

GlobalAddress RadixTree::get_root_ptr_ptr() {
  GlobalAddress addr;
  addr.nodeID = 0;
  addr.offset = define::kRootPointerStoreOffest;
  return addr;
}

GlobalAddress RadixTree::get_root_ptr(CoroContext *cxt, int coro_id) {
  auto page_buffer = (dsm->get_rbuf(0)).get_page_buffer();
  dsm->read_sync(page_buffer, root_ptr_ptr, sizeof(GlobalAddress), cxt);
  GlobalAddress root_ptr = *(GlobalAddress *)page_buffer;
  return root_ptr;
}

void RadixTree::insert(const Key &k, const Value &v, CoroContext *cxt, int coro_id) {
  assert(dsm->is_register());
  GlobalAddress root_addr = get_root_ptr(cxt, coro_id);

  auto &rbuf = dsm->get_rbuf(coro_id);
  uint64_t *cas_buffer = rbuf.get_cas_buffer();
  auto page_buffer = rbuf.get_page_buffer();

  dsm->read_sync(page_buffer, root_addr, kLeafPageSize, cxt);
  auto page = (LeafNode *)page_buffer;
  assert(page->check_consistent());

  char *update_addr = nullptr;
  for (int i=0; i<4; i++) {
    auto &r = page->records[i];
    if (r.value == kValueNull) {
      r.key = k;
      r.value = v;
      r.f_version++;
      r.r_version = r.f_version;
      update_addr = (char *)&r;
      break;
    }
  }

  if (update_addr == nullptr) {
    std::cout<<"no free slot"<<std::endl;
    return;
  }

  auto dst_addr = GADD(root_addr, (update_addr - (char*)page));
  dsm->write_sync(update_addr, dst_addr, sizeof(LeafEntry), cxt);
  return;
}

bool RadixTree::search(const Key &k, Value &v, CoroContext *cxt, int coro_id) {
  assert(dsm->is_register());

  auto root_addr = get_root_ptr(cxt, coro_id);
  auto page_buffer = (dsm->get_rbuf(coro_id)).get_page_buffer();
  dsm->read_sync(page_buffer, root_addr, kLeafPageSize, cxt);

  auto page = (LeafNode *)page_buffer;
  for (int i=0; i<4; i++) {
    auto &r = page->records[i];
    if (r.key == k) {
      v = r.value;
      return true;
    }
  }

  return false;
}

GlobalAddress RadixTree::search(const VarKey &k, CoroContext *cxt = nullptr, int coro_id = 0) {
  assert(dsm->is_register());
  GlobalAddress root_addr = get_root_ptr(cxt, coro_id);
  auto &rbuf = dsm->get_rbuf(coro_id);

  GlobalAddress node_addr = root_addr;
  N *node = read_node_sync(rbuf, node_addr, cxt);
  uint32_t level = 0;
  bool optimisticPrefixMatch = false;

  while (true) {
    switch (checkPrefix(node, k, level)) {
      case CheckPrefixResult::NoMatch:
        return GlobalAddress::Null();
      case CheckPrefixResult::OptimisticMatch:
        optimisticPrefixMatch = true;
      case CheckPrefixResult::Match: {
        if (k.getKeyLen() <= level) {
          return GlobalAddress::Null();
        }
        //node_addr = N::getChild(k[level], node);
        node_addr = searchNode(k[level], node);
        if (node_addr == GlobalAddress::Null()) {
          return GlobalAddress::Null();
        }
        if (N::isLeaf(node_addr)) {
          return toDSMAddr(node_addr);
        }
        node = read_node_sync(rbuf, node_addr, cxt);
      }
    }
    level++;
  }

}

GlobalAddress RadixTree::searchNode(uint8_t key, N *node) {
  NTypes node_type = node->getType();
  uint32_t num_entries;
  NEntry *entries;
  switch (node_type) {
    case NTypes::N4:
      num_entries = 4;
      auto n = (N4*)node;
      entries = n->entries;
      break;
    case NTypes::N16:
      num_entries = 16;
      auto n = (N16*)node;
      entries = n->entries;
      break;
    case NTypes::N48:
      num_entries = 48;
      auto n = (N48*)node;
      entries = n->entries;
      break;
    case NTypes::N256:
      auto n = (N256 *)node;
      return n->entries[key].pointer;
  }
  for (int i=0; i<num_entries; i++) {
    if (entries[i].pointer.rNChar == key) {
      return entries[i].pointer;
    }
  }
  return GlobalAddress::Null();
}

void RadixTree::insert(const VarKey &k, GlobalAddress data_address, CoroContext *cxt = nullptr, int coro_id = 0) {
  assert(dsm->is_register());
  GlobalAddress root_addr = get_root_ptr(cxt, coro_id);
  auto &rbuf = dsm->get_rbuf(coro_id);
  // uint64_t *cas_buffer = rbuf.get_cas_buffer();
  // auto page_buffer = rbuf.get_page_buffer();

restart:
  bool needRestart = false;
  N *node = nullptr;
  GlobalAddress node_addr = GlobalAddress::Null();
  N *nextNode = read_node_sync(rbuf, root_addr, cxt);
  GlobalAddress next_node_addr = root_addr;
  N *parentNode = nullptr;
  GlobalAddress parent_node_addr = GlobalAddress::Null();

  uint8_t parentKey, nodeKey = 0;
  uint32_t level;

  while (true) {
    parentNode = node;
    parent_node_addr = node_addr;
    parentKey = nodeKey;
    node = nextNode;
    node_addr = next_node_addr;

    uint32_t nextLevel = level;
    uint8_t nonMatchingKey;
    Prefix remainingPrefix;
    switch (checkPrefixPessimistic(node, k, nextLevel, nonMatchingKey, remainingPrefix, this->loadKey)) {
      case CheckPrefixPessimisticResult::SkippedLevel:
        goto restart;
      case CheckPrefixPessimisticResult::NoMatch: {
        assert(nextLevel < k.getKeyLen());
        lockVersionOrRestart(rbuf, node_addr, node, needRestart, cxt);
        if (needRestart) goto restart;

        //what if parent is null?
        writeLockOrRestart(rbuf, parent_node_addr, parentNode, needRestart, cxt);
        if (needRestart) {
          writeUnlock(node, node_addr, cxt);
          goto restart;
        }

        Prefix prefix = node->getPrefix();
        prefix.prefixCount = nextLevel - level;
        GlobalAddress new_node_addr = dsm->alloc(sizeof(N4));
        new_node_addr.rIsLeaf = 0;
        new_node_addr.rNChar = parentKey;
        new_node_addr.rNType = static_cast<uint64_t>(NTypes::N4);

        auto new_node_buff = rbuf.get_sibling_buffer();
        auto new_node = new (new_node_buff) N4(nextLevel, prefix);

        new_node->insert(k[nextLevel], N::setLeaf(data_address));
        new_node->insert(nonMatchingKey, node_addr);
        
        //node and parent node are locked
        changeNode(parentNode, parentKey, new_node_addr);
        node->setPrefix(remainingPrefix.prefix,
                        node->getPrefix().prefixCount - ((nextLevel - level) + 1));
        write_node_sync(node, N::getNodeSize(node), node_addr, cxt);
        write_node_sync(new_node, N::getNodeSize(new_node), new_node_addr, cxt);
        write_node_sync(parentNode, N::getNodeSize(parentNode), parent_node_addr, cxt);
        writeUnlock(parentNode, parent_node_addr, cxt);
        writeUnlock(node, node_addr, cxt);
        return;
      }
      case CheckPrefixPessimisticResult::Match:
        break;
    }
    //here to continue
    assert(nextLevel < k.getKeyLen());
    level = nextLevel;
    nodeKey = k[level];
    next_node_addr = searchNode(nodeKey, node);

    if (next_node_addr == GlobalAddress::Null()) {
      lockVersionOrRestart(rbuf, node_addr, node, needRestart, cxt);
      if (needRestart) goto restart;
      if (!insertNode(node, nodeKey, data_address)) {
        //node is full, need expand
        writeLockOrRestart(rbuf, parent_node_addr, parentNode, needRestart, cxt);
        if (needRestart) {
          writeUnlock(node, node_addr, cxt);
          goto restart;
        }
        GlobalAddress new_node_addr = GlobalAddress::Null();
        N *new_node = nullptr;
        switch (node->getType()) {
          case NTypes::N4:
            new_node_addr = dsm->alloc(sizeof(N16));
            new_node_addr.rIsLeaf = 0;
            new_node_addr.rNChar = parentKey;
            new_node_addr.rNType = static_cast<uint64_t>(NTypes::N16);
            auto new_node_buff = rbuf.get_sibling_buffer();
            new_node = new (new_node_buff) N16(node->getLevel(), node->getPrefix());
            auto n = (N4*)node;
            n->copyTo(new_node);
            break;
          case NTypes::N16:
            new_node_addr = dsm->alloc(sizeof(N48));
            new_node_addr.rIsLeaf = 0;
            new_node_addr.rNChar = parentKey;
            new_node_addr.rNType = static_cast<uint64_t>(NTypes::N48);
            auto new_node_buff = rbuf.get_sibling_buffer();
            new_node = new (new_node_buff) N48(node->getLevel(), node->getPrefix());
            auto n = (N16*)node;
            n->copyTo(new_node);
            break;
          case NTypes::N48:
            new_node_addr = dsm->alloc(sizeof(N256));
            new_node_addr.rIsLeaf = 0;
            new_node_addr.rNChar = parentKey;
            new_node_addr.rNType = static_cast<uint64_t>(NTypes::N256);
            auto new_node_buff = rbuf.get_sibling_buffer();
            new_node = new (new_node_buff) N256(node->getLevel(), node->getPrefix());
            auto n = (N48*)node;
            n->copyTo(new_node);
            break;
          case NTypes::N256:
            assert(false);
            break;
        }
        changeNode(parentNode, parentKey, new_node_addr);
        write_node_sync(new_node, N::getNodeSize(new_node), new_node_addr, cxt);
        write_node_sync(parentNode, N::getNodeSize(parentNode), parent_node_addr, cxt);
        writeUnlock(parentNode, parent_node_addr, cxt);
        writeUnlockObsolete(node, node_addr, cxt);
      } else {
        writeUnlock(node, node_addr, cxt);
      }
      if (needRestart) goto restart;
      return;
    }

    if (N::isLeaf(next_node_addr)) {
      lockVersionOrRestart(rbuf, node_addr, node, needRestart, cxt);
      if (needRestart) goto restart;

      VarKey key;
      loadVarKey(rbuf, next_node_addr, key, cxt);

      

    }
    level++;
  }
}

void RadixTree::loadVarKey(RdmaBuffer& rbuf, GlobalAddress addr, VarKey &key, CoroContext *cxt) {
  auto page_buffer = rbuf.get_page_buffer();
  addr = toDSMAddr(addr);
  dsm->read_sync(page_buffer, addr, KVBlockSize, cxt);
  key.set((char*)page_buffer, KeyBytes);
}

void RadixTree::writeUnlockObsolete(N *node, GlobalAddress addr, CoroContext *cxt) {
  uint64_t version = node->getVersion();
  version = version + 0b11; //unlock and inc version
  addr = toDSMAddr(addr);
  dsm->write_sync((char *)version, addr, sizeof(uint64_t), cxt);
}

bool RadixTree::insertNode(N *node, uint8_t key, GlobalAddress new_addr) {
  switch (node->getType()) {
    case NTypes::N4: {
      auto n = static_cast<N4 *>(node);
      for (int i=0; i<4; i++) {
        if (n->entries[i].pointer == GlobalAddress::Null()) {
          n->entries[i].pointer = N::setLeaf(new_addr);
          return true;
        }
      }
      break;
    }
    case NTypes::N16: {
      auto n = static_cast<N16 *>(node);
      for (int i=0; i<16; i++) {
        if (n->entries[i].pointer == GlobalAddress::Null()) {
          n->entries[i].pointer = N::setLeaf(new_addr);
          return true;
        }
      }
      break;
    }
    case NTypes::N48: {
      auto n = static_cast<N48 *>(node);
      for (int i=0; i<48; i++) {
        if (n->entries[i].pointer == GlobalAddress::Null()) {
          n->entries[i].pointer = N::setLeaf(new_addr);
          return true;
        }
      }
      break;
    }
    case NTypes::N256: {
      auto n = static_cast<N256 *>(node);
      n->entries[key].pointer = N::setLeaf(new_addr);
      return true;
    }
  }
  return false;
}

void RadixTree::changeNode(N *node, uint8_t key, GlobalAddress new_node_addr) {
  NTypes node_type = node->getType();
  uint32_t num_entries;
  NEntry *entries;
  switch (node_type) {
    case NTypes::N4:
      num_entries = 4;
      auto n = (N4*)node;
      entries = n->entries;
      break;
    case NTypes::N16:
      num_entries = 16;
      auto n = (N16*)node;
      entries = n->entries;
      break;
    case NTypes::N48:
      num_entries = 48;
      auto n = (N48*)node;
      entries = n->entries;
      break;
    case NTypes::N256:
      auto n = (N256 *)node;
      n->entries[key].front_version++;
      n->entries[key].pointer = new_node_addr;
      n->entries[key].rear_version = n->entries[key].front_version;
      return;
  }
  for (int i=0; i<num_entries; i++) {
    if (entries[i].pointer.rNChar == key) {
      entries[i].front_version++;
      entries[i].pointer = new_node_addr;
      entries[i].rear_version = entries[i].front_version;
      return;
    }
  }
  assert(false);
}

void RadixTree::writeUnlock(N *node, GlobalAddress addr, CoroContext *cxt) {
  uint64_t version = node->getVersion();
  version = version + 0b10; //unlock and inc version
  addr = toDSMAddr(addr);
  dsm->write_sync((char *)version, addr, sizeof(uint64_t), cxt);
}

void RadixTree::write_node_sync(N *node, uint64_t size, GlobalAddress addr, CoroContext *cxt) {
  dsm->write_sync((char *)node, addr, size, cxt);
}

void RadixTree::writeLockOrRestart(RdmaBuffer& rbuf, GlobalAddress addr, N *node, bool &needRestart, CoroContext *cxt) {
  auto cas_buf = rbuf.get_cas_buffer();
  uint64_t tag;// = node->getVersion() & 0b10;
  addr = toDSMAddr(addr);
  do {
    while (node->isLocked()) {
      dsm->read_sync((char *)&tag, addr, sizeof(uint64_t), cxt);
      node->setVersion(tag);
    }
    if (node->isObsolete()) {
      needRestart = true;
      return;
    }
    tag = node->getVersion() + 0b10;
  } while (!dsm->cas_dm_sync(addr, node->getVersion(), tag, cas_buf, cxt));
  node->setVersion(tag);
}

GlobalAddress RadixTree::to_embedded_address(GlobalAddress origin_address, uint8_t key, NTypes node_type) {
  GlobalAddress embedded_address = origin_address;
  embedded_address.rNChar = key;
  embedded_address.rNType = static_cast<uint64_t>(node_type);
  return embedded_address;
}

void RadixTree::lockVersionOrRestart(RdmaBuffer& rbuf, GlobalAddress addr, N *node, bool &needRestart, CoroContext *cxt) {
  if (node->isLocked() || node->isObsolete()) {
    needRestart = true;
    return;
  }
  uint64_t *cas_buf = rbuf.get_cas_buffer();
  uint64_t tag = node->getVersion() + 0b10;
  addr = toDSMAddr(addr);
  if (dsm->cas_dm_sync(addr, node->getVersion(), tag, cas_buf, cxt)) {
    node->setVersion(tag);
  } else {
    needRestart = true;
  }
}

N* RadixTree::read_node_sync(RdmaBuffer& rbuf, GlobalAddress addr, CoroContext *cxt) {
  auto page_buffer = rbuf.get_page_buffer();
  uint64_t read_size = 0;
  NTypes node_type = static_cast<NTypes>(addr.rNType);
  switch (node_type) {
    case NTypes::N4:
      read_size = sizeof(N4);
      break;
    case NTypes::N16:
      read_size = sizeof(N16);
      break;
    case NTypes::N48:
      read_size = sizeof(N48);
      break;
    case NTypes::N256:
      read_size = sizeof(N256);
      break;
  }
  addr = toDSMAddr(addr);
  N *tmp;
  while (true) {
    dsm->read_sync(page_buffer, addr, read_size, cxt);
    tmp = (N *)page_buffer;
    if (N::check_consistent(tmp)) {
      break;
    }
  }
  return tmp;
}

CheckPrefixResult RadixTree::checkPrefix(N *n, const VarKey &k, uint32_t &level) {
  if (k.getKeyLen() < n->getLevel()) {
    return CheckPrefixResult::NoMatch;
  }
  Prefix p = n->getPrefix();
  if (p.prefixCount + level < n->getLevel()) {
    level = n->getLevel();
    return CheckPrefixResult::OptimisticMatch;
  }
  if (p.prefixCount > 0) {
    for (uint32_t i = ((level + p.prefixCount) - n->getLevel());
          i < std::min(uint32_t(p.prefixCount), maxStoredPrefixLength); ++i) {
      if (p.prefix[i] != k[level]) {
        return CheckPrefixResult::NoMatch;
      }            
      ++level;
    }
    if (p.prefixCount > maxStoredPrefixLength) {
      level += p.prefixCount - maxStoredPrefixLength;
      return CheckPrefixResult::OptimisticMatch;
    }
  }
  return CheckPrefixResult::Match;
}

CheckPrefixPessimisticResult RadixTree::checkPrefixPessimistic(N *n, const VarKey &k, uint32_t &level,
                                                                uint8_t &nonMatchingKey,
                                                                Prefix &nonMatchingPrefix,
                                                                LoadKeyFunction loadKey) {
  Prefix p = n->getPrefix();
  if (p.prefixCount + level < n->getLevel()) {
    return CheckPrefixPessimisticResult::SkippedLevel;
  }
  if (p.prefixCount > 0) {
    uint32_t prevLevel = level;
    VarKey kt;
    for (uint32_t i = ((level + p.prefixCount) - n->getLevel()); i < p.prefixCount; ++i) {
      if (i == maxStoredPrefixLength) {
        //loadKey todo
        assert(false);
      }
      uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : p.prefix[i];
      if (curKey != k[level]) {
        nonMatchingKey = curKey;
        if (p.prefixCount > maxStoredPrefixLength) {
          if (i < maxStoredPrefixLength) {
            //loadkey todo
            assert(false);
          }
          for (uint32_t j = 0; j < std::min((p.prefixCount - (level - prevLevel) - 1),
                                                          maxStoredPrefixLength); ++j) {
            nonMatchingPrefix.prefix[j] = kt[level + j + 1];
          }
        } else {
          for (uint32_t j = 0; j < p.prefixCount - i - 1; ++j) {
            nonMatchingPrefix.prefix[j] = p.prefix[i + j + 1];
          }
        }
        return CheckPrefixPessimisticResult::NoMatch;
      }
      ++level;
    }
  }
  return CheckPrefixPessimisticResult::Match;
}

PCCompareResults RadixTree::checkPrefixCompare(const N* n, const VarKey &k, uint32_t &level, LoadKeyFunction loadKey) {
  Prefix p = n->getPrefix();
  if (p.prefixCount + level < n->getLevel()) {
    return PCCompareResults::SkippedLevel;
  }
  if (p.prefixCount > 0) {
    VarKey kt;
    for (uint32_t i = ((level + p.prefixCount) - n->getLevel()); i < p.prefixCount; ++i) {
      if (i == maxStoredPrefixLength) {
        //loadkey todo
        assert(false);
      }
      uint8_t kLevel = (k.getKeyLen() > level) ? k[level] : 0;
      uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : p.prefix[i];
      if (curKey < kLevel) {
        return PCCompareResults::Smaller;
      } else if (curKey > kLevel) {
        return PCCompareResults::Bigger;
      }
      ++level;
    }
  }
  return PCCompareResults::Equal;
}

PCEqualsResults RadixTree::checkPrefixEquals(const N* n, uint32_t &level, const VarKey &start, const VarKey &end, LoadKeyFunction loadKey) {
  Prefix p = n->getPrefix();
  if (p.prefixCount + level < n->getLevel()) {
    return PCEqualsResults::SkippedLevel;
  }
  if (p.prefixCount > 0) {
    VarKey kt;
    for (uint32_t i = ((level + p.prefixCount) - n->getLevel()); i < p.prefixCount; ++i) {
      if (i == maxStoredPrefixLength) {
        //loadKey todo
        assert(false);
      }
      uint8_t startLevel = (start.getKeyLen() > level) ? start[level] : 0;
      uint8_t endLevel = (end.getKeyLen() > level) ? end[level] : 0;

      uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : p.prefix[i];
      if (curKey > startLevel && curKey < endLevel) {
        return PCEqualsResults::Contained;
      } else if (curKey < startLevel || curKey > endLevel) {
        return PCEqualsResults::NoMatch;
      }
      ++level;
    }
  }
  return PCEqualsResults::BothMatch;
}