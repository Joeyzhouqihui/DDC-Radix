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
  for (uint32_t i = 0; i < 256; ++i) {
    GlobalAddress addr = entries[i].pointer;
    if (addr != GlobalAddress::Null()) {
      addr.rNType = node_type;
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

