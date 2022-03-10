#ifndef __GLOBALADDRESS_H__
#define __GLOBALADDRESS_H__

#include "Common.h"


class GlobalAddress {
public:

union {
  struct {
    uint64_t nodeID : 16;
    uint64_t offset : 48;
  };
  struct {
    uint64_t rNType : 2;
		uint64_t rNChar : 8;
		uint64_t rNodeID : 6;
    uint64_t rIsLeaf : 1;
		uint64_t rOffset : 47;
	};
  uint64_t val;
};

 operator uint64_t() {
  return val;
}

  static GlobalAddress Null() {
    static GlobalAddress zero{0, 0};
    return zero;
  };
} __attribute__((packed));

static_assert(sizeof(GlobalAddress) == sizeof(uint64_t), "XXX");

inline GlobalAddress toDSMAddr(GlobalAddress addr) {
  addr.rNType = 0;
  addr.rNChar = 0;
  addr.rIsLeaf = 0;
  return addr;
}

inline GlobalAddress GADD(const GlobalAddress &addr, int off) {
  auto ret = addr;
  ret.offset += off;
  return ret;
}

inline bool operator==(const GlobalAddress &lhs, const GlobalAddress &rhs) {
  return (lhs.nodeID == rhs.nodeID) && (lhs.offset == rhs.offset);
}

inline bool operator!=(const GlobalAddress &lhs, const GlobalAddress &rhs) {
  return !(lhs == rhs);
}

inline std::ostream &operator<<(std::ostream &os, const GlobalAddress &obj) {
  os << "[" << (int)obj.nodeID << ", " << obj.offset << "]";
  return os;
}

#endif /* __GLOBALADDRESS_H__ */
