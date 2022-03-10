#include "N.h"
#include <algorithm>
#include <assert.h>

void N::setType(NTypes type) {
  typeVersionLockObsolete += convertTypeToVersion(type);
}

uint64_t N::convertTypeToVersion(NTypes type) {
  return (static_cast<uint64_t>(type) << 62);
}

NTypes N::getType() const {
  return static_cast<NTypes>(typeVersionLockObsolete >> 62);
}

uint32_t N::getLevel() const {
  return level;
}

uint32_t N::getCount() const {
  return count;
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

GlobalAddress N::getChild(const uint8_t k, N *node) {
  switch (node->getType()) {
    case NTypes::N4: {
      auto n = static_cast<N4*>(node);
      return n->getChild(k);
    }
    case NTypes::N16: {
      auto n = static_cast<N16*>(node);
      return n->getChild(k);
    }
    case NTypes::N48: {
      auto n = static_cast<N48*>(node);
      return n->getChild(k);
    }
    case NTypes::N256: {
      auto n = static_cast<N256*>(node);
      return n->getChild(k);
    }
  }
}

Prefix N::getPrefix() const {
  return prefix;
}

void N::setPrefix(const uint8_t *prefix, uint32_t length) {
  if (length > 0) {
    Prefix p;
    memcpy(p.prefix, prefix, std::min(length, maxStoredPrefixLength));
    p.prefixCount = length;
    this->prefix = p;
  } else {
    Prefix p;
    p.prefixCount = 0;
    this->prefix = p;
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

