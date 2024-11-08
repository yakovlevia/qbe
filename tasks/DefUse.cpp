#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <map>
#include <algorithm>

#define export exports
extern "C" {
    #include "qbe/all.h"
}
#undef export

static void readfn (Fn *fn) {
  std::vector<std::set<std::string>> def;
  std::vector<std::set<std::string>> use;
  std::string a = "@";
  std::string x = "%";
  std::map<int, Blk*> blocks;
  std::map<Blk*, int> blocks_names;

  int cnt = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    def.push_back(std::set<std::string>());
    use.push_back(std::set<std::string>());
    blocks[cnt] = blk;
    blocks_names[blk] = cnt;
    cnt++;
  }

  for (Blk *blk = fn->start; blk; blk = blk->link) {
    std::map<std::string, bool> has;
    for (int i = 0; i < blk->nins; i++) {
      Ins *current_ins = &blk->ins[i];

      std::string name = x + fn->tmp[current_ins->arg[0].val].name;
      if (name.size() > 1 && !has[name]) {
        use[blocks_names[blk]].insert(name);
      }
      
      name = x + fn->tmp[current_ins->arg[1].val].name;
      if (name.size() > 1 && !has[name]) {
        use[blocks_names[blk]].insert(name);
      }
    
      std::string line = x + fn->tmp[current_ins->to.val].name;
      if (line.size() > 1) {
        def[blocks_names[blk]].insert(line);
        has[line] = true;
      }
      
    }

    if (blk->jmp.type == Jretw) {
      std::string name = x + fn->tmp[blk->jmp.arg.val].name;
      if (name.size() > 1 && !has[name]) {
        use[blocks_names[blk]].insert(name);
      }
    }
  }

  int i = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tdef = ");
    for (auto q : def[i]) {
      std::cout << q << " ";
    }
    printf("\n\tuse = ");
    for (auto q : use[i]) {
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