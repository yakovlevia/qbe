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
  std::vector<std::set<std::string>>tmpgen;
  std::vector<std::set<std::string>>gen;
  std::vector<std::set<std::string>>kill;
  std::string a = "@";
  std::string x = "%";
  std::map<int, Blk*> blocks;
  std::map<Blk*, int> blocks_names;


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
    blocks[cnt] = blk;
    blocks_names[blk] = cnt;
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
  tmpgen.clear();
  
  std::vector<std::set<std::string>> input(gen.size());
  std::set<int> worklist;
  

  for (int i = 0; i < gen.size(); i++) {
    worklist.insert(i);
  }

  while (!worklist.empty()) {
    int pos = *worklist.begin();
    worklist.erase(worklist.begin());
    std::set<std::string> new_input;

    for (int i = 0; i < blocks[pos]->npred; i++) {
      int pred_id = blocks_names[blocks[pos]->pred[i]];
      std::set<std::string> temp = input[pred_id];
      std::vector<std::string> diff;

      std::set<std::string> ttemp;
      std::set_difference(temp.begin(), temp.end(), kill[pred_id].begin(), kill[pred_id].end(), std::inserter(ttemp, ttemp.begin()));
      temp = ttemp;
      std::set_union(temp.begin(), temp.end(), gen[pred_id].begin(), gen[pred_id].end(), std::inserter(temp, temp.begin()));
      std::set_union(new_input.begin(), new_input.end(), temp.begin(), temp.end(), std::inserter(new_input, new_input.begin()));

    }

    if (new_input != input[pos]) {
      input[pos] = new_input;
      if (blocks[pos]->s1) {
        worklist.insert(blocks_names[blocks[pos]->s1]);
      }
      if (blocks[pos]->s2) {
        worklist.insert(blocks_names[blocks[pos]->s2]);
      }
    }
  }
  
  int i = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\trd_in = ");
    for (auto q : input[i]) {
      std::cout << q << " ";
    }
    std::cout << "\n"; 
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