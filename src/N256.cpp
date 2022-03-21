#include "N.h"

bool N256::insert(uint8_t key, GlobalAddress val) {
  assert(static_cast<NTypes>(val.rNType) == NTypes::N256);
  assert(val.rNChar == key);
  if (entries[key].pointer != GlobalAddress::Null()) {
    return false;
  }
  entries[key].pointer = val;
  entries[key].front_version = 0;
  entries[key].rear_version = 0;
  return true;
}

template <class NODE>
void N256::copyTo(NODE *n) const {
  NTypes node_type = n->getType();
  uint8_t key;
  for (uint32_t i = 0; i < 256; ++i) {
    GlobalAddress addr = entries[i].pointer;
    if (addr != GlobalAddress::Null()) {
      addr.rNType = static_cast<uint64_t>(node_type);
      key = addr.rNChar;
      n->insert(key, addr);
    }
  }
}

bool N256::check_consistent() {
  return true;
}

// void N256::change(uint8_t key, GlobalAddress n) {
//   entries[key] = n;
// }

