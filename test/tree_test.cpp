#include "DSM.h"
#include "RadixTree.h"
//#include "Tree.h"

int main() {

  DSMConfig config;
  config.machineNR = 1;
  DSM *dsm = DSM::getInstance(config);
 
  dsm->registerThread();

  auto radix_tree = new RadixTree(dsm);
  Value v;

  for (int i=0; i<5; i++) {
    radix_tree->insert(i, 1235467);
  }

  for (int i=0; i<4; i++) {
    radix_tree->search(i, v);
    std::cout<<v<<std::endl;
  }

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