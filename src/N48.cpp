#include "N.h"

bool N48::insert(uint8_t key, GlobalAddress val) {
  assert(static_cast<NTypes>(val.rNType) == NTypes::N48);
  assert(val.rNChar == key);
  for (int i=0; i<48; i++) {
    if (entries[i].pointer == GlobalAddress::Null()) {
      entries[i].pointer = val;
      entries[i].front_version = 0;
      entries[i].rear_version = 0;
      return true;
    }
  }
  return false;
}

template <class NODE>
void N48::copyTo(NODE *n) const {
  NTypes node_type = n->getType();
  uint8_t key;
  for (uint32_t i = 0; i < 48; ++i) {
    GlobalAddress addr = entries[i].pointer;
    if (addr != GlobalAddress::Null()) {
      addr.rNType = static_cast<uint64_t>(node_type);
      key = addr.rNChar;
      n->insert(key, addr);
    }
  }
}

bool N48::check_consistent() {
  return true;
}

// void N48::change(uint8_t key, GlobalAddress val) {
//   for (uint32_t i = 0; i < compactCount; ++i) {
//     GlobalAddress addr = entries[i];
//     if (addr != GlobalAddress::Null() && addr.rNChar == key) {
//       entries[i] = addr;
//       return;
//     }
//   }
//   assert(false);
// }

// GlobalAddress N48::getChild(const uint8_t k) const {
//   for (uint32_t i = 0; i < 48; i++) {
//     GlobalAddress addr = entries[i];
//     if (addr != GlobalAddress::Null() && addr.rNChar == k) {
//       return addr;
//     }
//   }
//   return GlobalAddress::Null();
// }