#include "N.h"

bool N4::insert(uint8_t key, GlobalAddress val) {
  assert(static_cast<NTypes>(val.rNType) == NTypes::N4);
  assert(val.rNChar == key);
  for (int i=0; i<4; i++) {
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
void N4::copyTo(NODE *n) const {
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
  for (uint32_t i = 0; i < 4; ++i) {
    GlobalAddress addr = entries[i].pointer;
    if (addr != GlobalAddress::Null()) {
      addr.rNType = node_type;
      key = addr.rNChar;
      n->insert(key, addr);
    }
  }
}

bool N4::check_consistent() {
  return true;
}