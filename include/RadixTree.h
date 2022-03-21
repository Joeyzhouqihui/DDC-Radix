#ifndef RADIX_TREE_H
#define RADIX_TREE_H

#include "DSM.h"
#include "N.h"
#include "Key.h"
#include <atomic>
#include <city.h>
#include <functional>
#include <iostream>

enum class CheckPrefixResult : uint8_t {
  Match,
  NoMatch,
  OptimisticMatch
};

enum class CheckPrefixPessimisticResult : uint8_t {
  Match,
  NoMatch,
  SkippedLevel
};

enum class PCCompareResults : uint8_t {
  Smaller,
  Equal,
  Bigger,
  SkippedLevel
};
        
enum class PCEqualsResults : uint8_t {
  BothMatch,
  Contained,
  NoMatch,
  SkippedLevel
};

using LoadKeyFunction = void (*)(TID tid, Key &key);

const uint32_t KeyBytes = 8;
const uint32_t ValueBytes = 8;
const uint32_t KVBlockSize = KeyBytes + ValueBytes;

class RadixTree {
public:
  RadixTree(DSM *dsm);
  void insert(const Key &k, const Value &v, CoroContext *cxt = nullptr, int coro_id = 0);
  bool search(const Key &k, Value &v, CoroContext *cxt = nullptr, int coro_id = 0);
  void insert(const VarKey &k, GlobalAddress tid, CoroContext *cxt = nullptr, int coro_id = 0);
  GlobalAddress search(const VarKey &k, CoroContext *cxt = nullptr, int coro_id = 0);
  GlobalAddress store(const VarKey &k, const uint64_t &v, CoroContext *cxt = nullptr, int coro_id = 0);
  uint64_t load(const VarKey &k, const GlobalAddress &kv_addr, CoroContext *cxt = nullptr, int coro_id = 0);

private:
  GlobalAddress get_root_ptr_ptr();
  GlobalAddress get_root_ptr(CoroContext *cxt, int coro_id);
  N* read_node_sync(RdmaBuffer& rbuf, GlobalAddress addr, CoroContext *cxt);
  void lockVersionOrRestart(RdmaBuffer& rbuf, GlobalAddress addr, N *node, bool &needRestart, CoroContext *cxt);
  void write_node_sync(N *node, uint64_t size, GlobalAddress addr, CoroContext *cxt);
  GlobalAddress to_embedded_address(GlobalAddress origin_address, uint8_t key, NTypes node_type);
  //need modify
  void writeLockOrRestart(RdmaBuffer& rbuf, GlobalAddress addr, N *node, bool &needRestart, CoroContext *cxt);
  void writeUnlock(N *node, GlobalAddress addr, CoroContext *cxt);
  void writeUnlockObsolete(N *node, GlobalAddress addr, CoroContext *cxt);
  
  char* insertNode(N *node, uint8_t key, GlobalAddress new_addr);
  GlobalAddress searchNode(uint8_t key, N *node);
  void changeNode(N *node, uint8_t key, GlobalAddress node_address);

  void loadVarKey(RdmaBuffer& rbuf, GlobalAddress addr, VarKey &key, CoroContext *cxt);

  static CheckPrefixResult checkPrefix(N* n, const VarKey &k, uint32_t &level);
  static CheckPrefixPessimisticResult checkPrefixPessimistic(N *n, const VarKey &k, uint32_t &level,
                                                              uint8_t &nonMatchingKey,
                                                              Prefix &nonMatchingPrefix,
                                                              LoadKeyFunction loadKey);
  static PCCompareResults checkPrefixCompare(const N* n, const VarKey &k, uint32_t &level, LoadKeyFunction loadKey);
  static PCEqualsResults checkPrefixEquals(const N* n, uint32_t &level, const VarKey &start, const VarKey &end, LoadKeyFunction loadKey);


private:
  DSM *dsm;
  GlobalAddress root_ptr_ptr;
  LoadKeyFunction loadKey;

};

class LeafHeader {
public:
  LeafHeader() {
    sibling_ptr = GlobalAddress::Null();
    level = 0;
    last_index = -1;
    lowest = kKeyMin;
    highest = kKeyMax;
  }

  GlobalAddress sibling_ptr;
  uint8_t level;
  int16_t last_index;
  Key lowest;
  Key highest;

} __attribute__((packed));

class LeafEntry {
public:
  uint8_t f_version;
  Key key;
  Value value;
  uint8_t r_version;

  LeafEntry() {
    f_version = 0;
    r_version = 0;
    value = kValueNull;
    key = 0;
  }

} __attribute__((packed));

class LeafNode {
public:
  LeafNode() {
    f_version = 0;
    r_version = 0;
    embedding_lock = 0;
  }

  void set_consistent() {
    f_version++;
    r_version = f_version;
  }

  bool check_consistent() {
    return (f_version == r_version);
  }

  uint64_t embedding_lock;
  uint8_t f_version;
  LeafHeader hdr;
  LeafEntry records[4];
  uint8_t r_version;

} __attribute__((packed));

#endif