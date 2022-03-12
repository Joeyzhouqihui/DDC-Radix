#ifndef RADIX_H
#define RADIX_H

#include "GlobalAddress.h"
#include <stdint.h>
#include <string.h>

using TID = uint64_t;

enum class NTypes {
  N4 = 0,
  N16 = 1,
  N48 = 2,
  N256 = 3
};

static constexpr uint32_t maxStoredPrefixLength = 4;

struct Prefix {
  uint32_t prefixCount = 0;
  uint8_t prefix[maxStoredPrefixLength];
} __attribute__((packed));

static_assert(sizeof(Prefix) == 8, "Prefix should be 64 bit long");

struct Header {
  uint8_t front_version;
  Prefix prefix;
  uint32_t level;
  uint8_t rear_version;
} __attribute__((packed));

class N {
public:
  N(NTypes type, uint32_t level, const uint8_t *prefix, uint32_t prefixLength) {
    header.front_version = 0;
    header.level = level;
    setType(type);
    setPrefix(prefix, prefixLength);
    header.rear_version = 0;
  }

  N(NTypes type, uint32_t level, const Prefix &prefix) : prefix(prefix), level(level) {
    header.front_version = 0;
    setType(type);
    header.rear_version = 0;
  }

  void setType(NTypes type);
  void setVersion(uint64_t version);
  void setPrefix(const uint8_t *prefix, uint32_t length);
  bool isLocked() const;
  bool isObsolete() const;
  NTypes getType() const;
  uint32_t getLevel() const;
  uint64_t getVersion() const;
  Prefix getPrefix() const;
  static uint64_t getNodeSize(N *node);
  static GlobalAddress getChild(const uint8_t k, N *node);
  static GlobalAddress getLeaf(const GlobalAddress n);
  static bool isLeaf(const GlobalAddress n);
  static GlobalAddress setLeaf(GlobalAddress tid);
  static void set_consistent(N *node);
  static bool check_consistent(N *node);

//differ local state change
  static void change(N *node, uint8_t key, N *val);

//need rdma op
  void writeLockOrRestart(bool &needRestart);
  void lockVersionOrRestart(uint64_t &version, bool &needRestart);
  static void insertAndUnlock(N *node, N *parentNode, uint8_t keyParent, uint8_t key, N *val, bool &needRestart);
  void writeUnlock();
  void writeUnlockObsolete();
  static N* getAnyChild(const N *n);
  static TID getAnyChildTid(const N *n);

  template<typename curN, typename biggerN>
  static void insertGrow(curN *n, N *parentNode, uint8_t keyParent, uint8_t key, N *val, bool &needRestart);
  
  template<typename curN>
  static void insertCompact(curN *n, N *parentNode, uint8_t keyParent, uint8_t key, N *val, bool &needRestart);

  uint64_t typeVersionLockObsolete{0};
  Header header;

} __attribute__((packed));

struct NEntry {
  uint8_t front_version;
  GlobalAddress pointer;
  uint8_t rear_version;
};

class N4 : public N {
public:
  N4(uint32_t level, const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N4, level, prefix, prefixLength) {
    for (int i=0; i<4; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  N4(uint32_t level, const Prefix &prefix) : N(NTypes::N4, level, prefix) {
    for (int i=0; i<4; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  bool insert(uint8_t key, GlobalAddress val);
  template <class NODE>
  void copyTo(NODE *n) const;
  void change(uint8_t key, GlobalAddress val);
  GlobalAddress getChild(const uint8_t k) const;
  bool check_consistent();

  NEntry entries[4];

};

class N16 : public N {
public:
  N16(uint32_t level, const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N16, level, prefix, prefixLength) {
    for (int i=0; i<16; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  N16(uint32_t level, const Prefix &prefi) : N(NTypes::N16, level, prefi) {
    for (int i=0; i<16; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  bool insert(uint8_t key, GlobalAddress n);
  template<class NODE>
  void copyTo(NODE *n) const;
  void change(uint8_t key, GlobalAddress val);
  GlobalAddress getChild(const uint8_t k) const;
  bool check_consistent();

  NEntry entries[16];

};

class N48 : public N {
public:
  N48(uint32_t level, const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N48, level, prefix, prefixLength) {
    for (int i=0; i<48; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  N48(uint32_t level, const Prefix &prefi) : N(NTypes::N48, level, prefi) {
    for (int i=0; i<48; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }    
  }

  bool insert(uint8_t key, GlobalAddress n);
  template<class NODE>
  void copyTo(NODE *n) const;
  void change(uint8_t key, GlobalAddress val);
  GlobalAddress getChild(const uint8_t k) const;
  bool check_consistent();

  NEntry entries[48];

};

class N256 : public N {
public:
  N256(uint32_t level, const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N256, level, prefix, prefixLength) {
    for (int i=0; i<256; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  N256(uint32_t level, const Prefix &prefi) : N(NTypes::N256, level, prefi) {
    for (int i=0; i<256; i++) {
      entries[i].front_version = 0;
      entries[i].pointer = GlobalAddress::Null();
      entries[i].rear_version = 0;
    }
  }

  bool insert(uint8_t key, GlobalAddress val);
  template<class NODE>
  void copyTo(NODE *n) const;
  void change(uint8_t key, GlobalAddress n);
  GlobalAddress getChild(const uint8_t k) const;
  bool check_consistent();

  NEntry entries[256];

};

#endif