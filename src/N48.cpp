#include "N.h"

bool N48::insert(uint8_t key, GlobalAddress val) {
  if (compactCount == 48) {
    return false;
  }
  assert(static_cast<NTypes>(val.rNType) == NTypes::N16);
  assert(val.rNChar == key);
  entries[compactCount] = val;
  compactCount++;
  count++;
  return true;
}

template <class NODE>
void N48::copyTo(NODE *n) const {
  NTypes node_type;
  switch (NODE) {
    case N4:
      node_type = NTypes::N4;
      break;
    case N16:
      node_type = NTypes::N16;
      break;
    case N48:
      node_type = NTypes::N48;
      break;
    case N256:
      node_type = NTypes::N256;
      break;
  }
  uint8_t key;
  for (uint32_t i = 0; i < compactCount; ++i) {
    GlobalAddress addr = entries[i];
    if (addr != GlobalAddress::Null()) {
      addr.rNType = node_type;
      key = addr.rNChar;
      n->insert(key, addr);
    }
  }
}

void N48::change(uint8_t key, GlobalAddress val) {
  for (uint32_t i = 0; i < compactCount; ++i) {
    GlobalAddress addr = entries[i];
    if (addr != GlobalAddress::Null() && addr.rNChar == key) {
      entries[i] = addr;
      return;
    }
  }
  assert(false);
}

GlobalAddress N48::getChild(const uint8_t k) const {
  for (uint32_t i = 0; i < 48; i++) {
    GlobalAddress addr = entries[i];
    if (addr != GlobalAddress::Null() && addr.rNChar == k) {
      return addr;
    }
  }
  return GlobalAddress::Null();
}