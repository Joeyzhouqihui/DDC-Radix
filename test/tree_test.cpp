#include "DSM.h"
#include "RadixTree.h"
#include <stdio.h>
//#include "Tree.h"

int main() {

  DSMConfig config;
  config.machineNR = 1;
  DSM *dsm = DSM::getInstance(config);
  dsm->registerThread();

  printf("env set up complete\n");

  auto radix_tree = new RadixTree(dsm);
  VarKey key;
  key.set("qhzhou", 6);
  uint32_t value = 100;
  GlobalAddress kv_addr = radix_tree->store(key, value);
  radix_tree->insert(key, kv_addr);
  
  GlobalAddress return_kv_addr = radix_tree->search(key);
  uint64_t return_value = radix_tree->load(key, return_kv_addr);
  printf("return value: %ld\n", return_value);
  // auto tree = new Tree(dsm);

  // Value v;

  // if (dsm->getMyNodeID() != 0) {
  //   while(true);
  // }


  // for (uint64_t i = 1; i < 1024; ++i) {
  //   tree->insert(i, i * 2);
  // }

  // for (uint64_t i = 1024 - 1; i >= 1; --i) {
  //   tree->insert(i, i * 3);
  // }

  // tree->insert(1234567, 7654321);

  // for (uint64_t i = 1; i < 1024; ++i) {
  //   auto res = tree->search(i, v);
  //   assert(res && v == i * 3);
  //   // std::cout << "search result:  " << res << " v: " << v << std::endl;
  // }


  // auto result = tree->search(1234567, v);

  // printf("Hello: %lu\n", v);

  // while (true)
  //   ;
}