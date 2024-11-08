#include <vector>
#include <set>
#include <string>
#include <iostream>

#define export exports
extern "C" {
    #include "qbe/all.h"
}
#undef export


static void readfn (Fn *fn) {
  std::vector<std::set<std::string>>tmpgen;
  std::vector<std::set<std::string>>gen;
  std::vector<std::set<std::string>>kill;
  std::string a = "@";
  std::string x = "%";


  for (Blk *blk = fn->start; blk; blk = blk->link) {
    gen.push_back(std::set<std::string>());
    tmpgen.push_back(std::set<std::string>());
    kill.push_back(std::set<std::string>());

    Ins* temp = blk->ins;
    for (int i = 0; i < blk->nins; i++) {
      Ins *current_ins = &blk->ins[i];
      if  (Tmp0 > current_ins->to.val) continue;
      tmpgen.back().insert(fn->tmp[current_ins->to.val].name);
      
      std::string line = a + blk->name + x + fn->tmp[current_ins->to.val].name;
      gen.back().insert(line);
    }
  }

  int cnt = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    Ins* temp = blk->ins;
    for (int j = 0; j < gen.size(); j++) {
      if (cnt == j) continue;
      for (int i = 0; i < blk->nins; i++) {
        Ins *current_ins = &blk->ins[i];
        if  (Tmp0 > current_ins->to.val) continue;
        if (tmpgen[j].find(fn->tmp[current_ins->to.val].name) != tmpgen[j].end()) {
          
          std::string line = a + blk->name + x + fn->tmp[current_ins->to.val].name;
          kill[j].insert(line);
        }
      }
    }
    cnt++;
  }

  int i = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tgen = ");
    for (auto q : gen[i]) {
      std::cout << q << " ";
    }
    printf("\n\tkill = ");
    for (auto q : kill[i]) {
      std::cout << q << " ";
    }
    printf("\n");
    i++;
  }

}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}