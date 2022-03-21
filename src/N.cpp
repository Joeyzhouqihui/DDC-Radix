#include "N.h"
#include "N4.cpp"
#include "N16.cpp"
#include "N48.cpp"
#include "N256.cpp"
#include <algorithm>
#include <assert.h>

void N::setType(NTypes type) {
  typeVersionLockObsolete += (static_cast<uint64_t>(type) << 62);
}

NTypes N::getType() const {
  return static_cast<NTypes>(typeVersionLockObsolete >> 62);
}

uint32_t N::getLevel() const {
  return header.level;
}

uint64_t N::getNodeSize(N *node) {
  switch (node->getType()) {
    case NTypes::N4: {
      return sizeof(N4);
    }
    case NTypes::N16: {
      return sizeof(N16);
    }
    case NTypes::N48: {
      return sizeof(N48);
    }
    case NTypes::N256: {
      return sizeof(N256);
    }
  }
}

bool N::isLocked() const {
  return ((typeVersionLockObsolete & 0b10) == 0b10);
}

uint64_t N::getVersion() const {
  return typeVersionLockObsolete;
}

void N::setVersion(uint64_t version) {
  typeVersionLockObsolete = version;
}

bool N::isObsolete() const {
  return (typeVersionLockObsolete & 1) == 1;
}

Prefix N::getPrefix() const {
  return header.prefix;
}

void N::setPrefix(const uint8_t *prefix, uint32_t length) {
  if (length > 0) {
    memcpy(header.prefix.prefix, prefix, std::min(length, maxStoredPrefixLength));
    header.prefix.prefixCount = length;
  } else {
    header.prefix.prefixCount = 0;
  }
}

//第48个bit为区分
GlobalAddress N::getLeaf(const GlobalAddress n) {
  GlobalAddress leaf = n;
  leaf.rIsLeaf = 0;
  return leaf;
}

bool N::isLeaf(const GlobalAddress n) {
  return n.rIsLeaf;
}

GlobalAddress N::setLeaf(GlobalAddress tid) {
  tid.rIsLeaf = 1;
  return tid;
}

void N::set_consistent(N *node) {
  node->header.front_version++;
  node->header.rear_version = node->header.front_version;
}

bool N::check_consistent(N *node) {
  bool success = 
    node->header.front_version == node->header.rear_version;
  if (!success) {
    return false;
  }
  switch (node->getType()) {
    case NTypes::N4: {
      N4 *n = static_cast<N4*>(node);
      return n->check_consistent();
    }
    case NTypes::N16: {
      N16 *n = static_cast<N16*>(node);
      return n->check_consistent();
    }
    case NTypes::N48: {
      N48 *n = static_cast<N48*>(node);
      return n->check_consistent();
    }
    case NTypes::N256: {
      N256 *n = static_cast<N256*>(node);
      return n->check_consistent();
    }
  }
  return success;
}
