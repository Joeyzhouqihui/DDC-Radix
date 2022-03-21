#include <stdio.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/coroutine/all.hpp>
#include <assert.h>

using Coroutine_t = boost::coroutines::symmetric_coroutine<void>;
using CoroYield = boost::coroutines::symmetric_coroutine<void>::yield_type;
using CoroCall = boost::coroutines::symmetric_coroutine<void>::call_type;

// int main()
// {
// 	Coroutine_t::call_type coro_recv(
// 		[&](Coroutine_t::yield_type& yield) {
// 		for (int i=0; i<3; i++) {
//          std::cout<<i<<std::endl;
//          (yield)(coro_send);
//       }
// 	});
//    Coroutine_t::call_type coro_send(
// 		[&](Coroutine_t::yield_type& yield) {
// 		for (int i=3; i<6; i++) {
//          std::cout<<i<<std::endl;
//          (yield)(coro_recv);
//       }
// 	});
// 	coro_send();
// 	return 0;
// }

class GlobalAddress {
public:

union {
  struct {
    uint64_t nodeID : 16;
    uint64_t offset : 48;
  };
  struct {
    uint64_t rNChar : 8;
    uint64_t rNType : 2;
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

enum class NTypes : uint8_t {
  N4 = 0,
  N16 = 1,
  N48 = 2,
  N256 = 3
};

int main() {
  uint64_t bit_mask = 0x0000800000000000;
  GlobalAddress n_addr;
  n_addr.val = 0;
  n_addr.rNChar = 12;
  printf("%ld\n", n_addr.rNChar);
  n_addr.rNType = static_cast<uint64_t>(NTypes::N256);
  n_addr.rNodeID = 0;
  NTypes type = NTypes::N256;
  uint64_t v = static_cast<uint64_t>(type) << 62;
  printf("%lu\n", v);
	return 0;
}