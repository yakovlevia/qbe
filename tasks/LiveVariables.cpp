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

  std::vector<std::set<std::string>> output(cnt);
  std::set<int> worklist;
  

  for (int i = 0; i < cnt; i++) {
    worklist.insert(i);
  }

  while (!worklist.empty()) {
    int pos = *worklist.begin();
    worklist.erase(worklist.begin());
    std::set<std::string> new_output;

    if (blocks[pos]->s1) {
      int succ_id = blocks_names[blocks[pos]->s1];
      std::set<std::string> temp = output[succ_id];

      std::set<std::string> ttemp;
      std::set_difference(temp.begin(), temp.end(), def[succ_id].begin(), def[succ_id].end(), std::inserter(ttemp, ttemp.begin()));
      temp = ttemp;
      std::set_union(temp.begin(), temp.end(), use[succ_id].begin(), use[succ_id].end(), std::inserter(temp, temp.begin()));
      std::set_union(new_output.begin(), new_output.end(), temp.begin(), temp.end(), std::inserter(new_output, new_output.begin()));
    }
    if (blocks[pos]->s2) {
      int succ_id = blocks_names[blocks[pos]->s2];
      std::set<std::string> temp = output[succ_id];

      std::set<std::string> ttemp;
      std::set_difference(temp.begin(), temp.end(), def[succ_id].begin(), def[succ_id].end(), std::inserter(ttemp, ttemp.begin()));
      temp = ttemp;
      std::set_union(temp.begin(), temp.end(), use[succ_id].begin(), use[succ_id].end(), std::inserter(temp, temp.begin()));
      std::set_union(new_output.begin(), new_output.end(), temp.begin(), temp.end(), std::inserter(new_output, new_output.begin()));
    }

    if (new_output != output[pos]) {
      output[pos] = new_output;
      
      for (int i = 0; i < blocks[pos]->npred; i++) {
        worklist.insert(blocks_names[blocks[pos]->pred[i]]);
      }
    }
  }
  
  int i = 0;
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\tlv_out = ");
    for (auto q : output[i]) {
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