#include <stdio.h>
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

static int useful(Ins* i) {
    //...
    return 1;
}

enum OperType {
    jmp,
    phi,
    ins,
    none,
};

struct WorklistItem {
    OperType type_st;
    
    Blk* blk_st;
    Ins* ins_st;
    Phi* phi_st;
};

bool operator < (const Ref a, const Ref b) {
    return a.type + 1000 * a.val < b.type + 1000 * b.val;
}

int dfs_id = 0;

void dfs(Blk* blk, std::map<Blk*, int> &ids) {
    if (ids[blk] == 0) {
        dfs_id++;
        ids[blk] = dfs_id;
        for (int i = 0; i < blk->npred; i++) {
            dfs(blk->pred[i], ids);
        }
    }
}

std::map<Blk*, int> get_idom_pos(Fn *fn) {
    std::map<Blk*, int> ids;

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        if (Jret0 <= blk->jmp.type && blk->jmp.type <= Jretc) {
            dfs(blk, ids);
        }
    }

    return ids;
}

Blk* get_idom(std::set<Blk*> &vert, std::map<Blk*, int> &ids, Blk* blk) {
    int id = 0;
    Blk* res = nullptr;

    for (auto q : vert) {
        if (blk == q) continue;
        if (ids[q] > id) {
            id = ids[q];
            res = q;
        }
    }

    return res;
}

void get_dom(Fn *fn, std::map<Blk*, std::set<Blk*>> &dom) {
    
    std::map<Blk*, std::set<Blk*>> pred;
    std::set<Blk*> N;

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        N.insert(blk);
        for (int i = 0; i < blk->npred; i++) {
            pred[blk].insert(blk->pred[i]);
        }
        if (blk->jmp.type == Jretw) {
            dom[blk] = {blk};
        }
    }

    bool flag = true;
    

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        if (dom[blk].size() == 0) {
            dom[blk] = N;
        }
    }

    while (flag) {
        flag = false;
        for (Blk* blk = fn->start; blk; blk = blk->link) { 
            std::set<Blk*> intersection = N;

            if (pred[blk].size() == 0) {
                intersection = {};
            }

            for (auto pr : pred[blk]) {
                std::set<Blk*> tmp;
                std::set_intersection(intersection.begin(), intersection.end(), dom[pr].begin(), dom[pr].end(), std::inserter(tmp, tmp.begin()));
                intersection = tmp;
            }

            intersection.insert(blk);
            if (dom[blk] != intersection) {
                dom[blk] = intersection;
                flag = true;
            }
        } 
    }
}

void get_rdf(Fn *fn, std::map<Blk*, std::set<Blk*>> &rdf, std::map<Blk*, std::set<Blk*>> &rdom, std::map<Blk*, int> &ids, std::map<Blk*, Blk*> &ridom) {
    
    std::map<Blk*, std::set<Blk*>> pred;

    std::set<Blk*> N;
    ids = get_idom_pos(fn);

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        N.insert(blk);
        for (int i = 0; i < blk->npred; i++) {
            pred[blk->pred[i]].insert(blk);
        }
        if (blk->jmp.type == Jretw) {
            rdom[blk] = {blk};
        }
    }

    bool flag = true;
    

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        if (rdom[blk].size() == 0) {
            rdom[blk] = N;
        }
    }

    while (flag) {
        flag = false;
        for (Blk* blk = fn->start; blk; blk = blk->link) { 
            std::set<Blk*> intersection = N;

            if (pred[blk].size() == 0) {
                intersection = {};
            }

            for (auto pr : pred[blk]) {
                std::set<Blk*> tmp;
                std::set_intersection(intersection.begin(), intersection.end(), rdom[pr].begin(), rdom[pr].end(), std::inserter(tmp, tmp.begin()));
                intersection = tmp;
            }

            intersection.insert(blk);
            if (rdom[blk] != intersection) {
                rdom[blk] = intersection;
                flag = true;
            }
        } 
    }

    for (auto [key, val] : rdom) {
        ridom[key] = get_idom(val, ids, key);
    }

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        if (pred[blk].size() < 2) continue;
        for (auto p : pred[blk]) {
            auto r = p;
            if (ridom[r]) {
                while (r != ridom[blk]) {
                    rdf[r].insert(blk);
                    r = ridom[r];
                }
            }
        }
    }

}

void print_info(std::set<Ins*> marked_i, std::set<Phi*> marked_phi, std::set<Blk*> marked_jmp,  std::set<Blk*> marked_blk,  std::map<Ref, std::vector<Ins*>> def, std::map<Ref, std::vector<Phi*>> def_phi, Fn *fn, std::map<Blk*, std::set<Blk*>> &rdom, std::map<Blk*, Blk*> &ridom, std::map<Blk*, std::set<Blk*>> &rdf){
    std::cout << "-----------Ins-------------------\n";

    for (auto item : marked_i) {
        std::cout << fn->tmp[item->to.val].name << " ";
    }

    std::cout << "\n-----------Phi-------------------\n";

    for (auto item : marked_phi) {
        std::cout << fn->tmp[item->to.val].name << " ";
    }

    std::cout << "\n-----------jump-------------------\n";

    for (auto item : marked_jmp) {
        std::cout << item->name << " ";
    }

    std::cout << "\n-----------jump--------------------\n";

    std::cout << "\n-----------blk--------------------\n";

    for (auto item : marked_blk) {
        std::cout << item->name << " ";
    }

    std::cout << "\n-----------blk--------------------\n";

    std::cout << "-----------rdom-------------------\n";
    for (auto [key, val] : rdom) {
        std::cout << key->name << " : ";
        for (auto q : val) {
            std::cout << q->name << " ";
        }
        std::cout << "\n";
    }
    std::cout << "-----------rdom-------------------\n";

    std::cout << "-----------ridom-------------------\n";
    for (auto [key, val] : ridom) {
        std::cout << key->name << " : ";
        std::cout << (val ? val->name : "null") << " ";
        std::cout << "\n";
    }
    std::cout << "-----------ridom-------------------\n";

    std::cout << "-----------rdf-------------------\n";
    for (auto [key, val] : rdf) {
        std::cout << key->name << " : ";
        for (auto q : val) {
            std::cout << q->name << " ";
        }
        std::cout << "\n";
    }
    std::cout << "-----------rdf-------------------\n";

    // for (auto [key, val] : def) {
    //     auto name = fn->tmp[key.val].name;
    //     std::cout << name << " : ";
    //     for (auto q : val) {
    //         std:: cout << q << " ";
    //     }
    //     std::cout << "\n";
    // }

    // std::cout << "\n-----------Def------------------\n";

    // for (auto [key, val] : def) {
    //     auto name = fn->tmp[key.val].name;
    //     std::cout << name << " : ";
    //     for (auto q : val) {
    //         std:: cout << q << " ";
    //     }
    //     std::cout << "\n";
    // }
    
    // std::cout << "\n-----------Def-------------------\n\n";

    // std::cout << "\n-----------Def_Phi------------------\n";

    // for (auto [key, val] : def_phi) {
    //     auto name = fn->tmp[key.val].name;
    //     std::cout << name << " : ";
    //     for (auto q : val) {
    //         std:: cout << q << " ";
    //     }
    //     std::cout << "\n";
    // }
    
    // std::cout << "\n-----------Def_Phi-------------------\n\n";
}


static void sweep(Fn *fn, std::set<Ins*> marked_i, std::set<Phi*> marked_phi, std::set<Blk*> marked_jmp, std::set<Blk*> marked_blk, std::map<Blk*, Blk*> ridom) {
    for (Blk* blk = fn->start; blk; blk = blk->link) {
        int cnt = 0;
        for (Ins *i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            if (marked_i.find(i) == marked_i.end()) {
                i->op = Onop;
                i->to = R;
                i->arg[0] = R;
                i->arg[1] = R;
            }
            else {
                cnt++;
            }
        }

        Phi* prev_phi = NULL;
        for (Phi *i = blk->phi; i;) {
            if (marked_phi.find(i) == marked_phi.end()) {
                
                if (prev_phi == NULL) {
                    blk->phi = i->link;
                    i = i->link;
                } else {
                    prev_phi->link = i->link;
                    i = i->link;
                }
            } 
            else {
                prev_phi = i;
                i = i->link;
                cnt++;
            }
        }

        if (marked_jmp.find(blk) != marked_jmp.end()) {
            continue;
        }

        if (Jjmp == blk->jmp.type || Jjnz == blk->jmp.type) {
            if (marked_jmp.find(blk->s1) == marked_jmp.end() && marked_blk.find(blk->s1) != marked_blk.end()) {
               continue;
            }
            
            if (ridom[blk]) {
                blk->jmp.type = Jjmp;
                blk->jmp.arg = R;

                Blk* probe = ridom[blk];
                while (probe && marked_jmp.find(probe) == marked_jmp.end()) {
                    probe = ridom[probe];
                }

                blk->s1 = probe;
                blk->s2 = nullptr;
            }
        }
    }

}

static void mark_sweep(Fn *fn) {
    std::vector<WorklistItem> worklist;
    std::set<Ins*> marked_i;
    std::set<Phi*> marked_phi;
    std::set<Blk*> marked_jmp;
    std::map<Ref, std::vector<Ins*>> def;
    std::map<Ref, std::vector<Phi*>> def_phi;
    std::map<Blk*, std::set<Blk*>> rdf;
    std::map<Blk*, std::set<Blk*>> equal;
    std::map<Blk*, std::set<Blk*>> rdom;
    std::map<Blk*, Blk*> ridom;
    std::set<Blk*> marked_blk;
    std::map<Blk*, int> ids;
    std::map<Blk*, std::set<Blk*>> dom;

    std::map<Ins*, Blk*> ins_to_blk;
    std::map<Phi*, Blk*> phi_to_blk;

    get_rdf(fn, rdf, rdom, ids, ridom);
    get_dom(fn, dom);

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        for (Blk* blk1 = fn->start; blk1; blk1 = blk1->link) { 
            if (blk1 == blk) continue;
            bool bi = dom[blk1].find(blk) == dom[blk1].end();
            Blk* bj = ridom[blk];
            if (!bj) {
                continue;
            }
            if (bi && bj == blk1) {
                //std::cout << blk->name << " " << blk1 << " bbb\n";
                equal[blk].insert(blk1);
                equal[blk1].insert(blk);
            }
        }
    }

    for (Blk* blk = fn->start; blk; blk = blk->link) { 
        //std::cout << blk->name << " blk\n";
        if (blk->jmp.type == Jjmp) {
            int cc = blk->s1->nins + (blk->phi != nullptr);
            //std::cout << blk->s1->name << " " << cc << "\n";
            if (equal[blk->s1].find(blk) != equal[blk->s1].end() && cc == 0 && !isret(blk->s1->jmp.type)) {
                //std::cout << blk->name << " name\n";
                marked_jmp.insert(blk);
            }
        }
    }
    

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        if (!ridom[blk]) {
            WorklistItem item = {.type_st = jmp, .blk_st = blk};
            worklist.push_back(item);
            marked_jmp.insert(blk);
        }
    }

    //WorklistItem item = {.type_st = jmp, .blk_st = fn->start};
    //worklist.push_back(item);
    //marked_jmp.insert(fn->start);

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        //std::cout << blk->name << ":\n";
        for (Ins *i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            ins_to_blk[i] = blk;
            
            //std::cout << "ins: " << i->op << " " << fn->tmp[i->to.val].name << " " << fn->tmp[i->arg[0].val].name << " " << fn->tmp[i->arg[1].val].name << "\n";

            if (i->op == 97) {
                WorklistItem item = {.type_st = ins, .blk_st = blk, .ins_st = i};
                marked_i.insert(i);
                worklist.push_back(item);

                if (def[i->arg[0]].empty()) {
                    def[i->arg[0]] = std::vector<Ins*>();
                }
                def[i->arg[0]].push_back(i);
            }
            
            if (i->op == Ovacall || i->op == Ocall || isstore(i->op)) {
                WorklistItem item = {.type_st = ins, .blk_st = blk, .ins_st = i};
                marked_i.insert(i);
                worklist.push_back(item);

                if (i->to.val >= Tmp0) {
                    if (def[i->to].empty()) {
                        def[i->to] = std::vector<Ins*>();
                    }
                    def[i->to].push_back(i);
                }

            }

            if (i->to.val >= Tmp0) {
                if (def[i->to].empty()) {
                    def[i->to] = std::vector<Ins*>();
                }
                def[i->to].push_back(i);
            }
        }

        for (Phi *i = blk->phi; i; i = i->link) {
            phi_to_blk[i] = blk;
            if (i->to.val >= Tmp0) {
                if (def_phi[i->to].empty()) {
                    def_phi[i->to] = std::vector<Phi*>();
                }
                def_phi[i->to].push_back(i);
            }
        }

        if (Jret0 <= blk->jmp.type && blk->jmp.type <= Jretc) {
            WorklistItem item = {.type_st = jmp, .blk_st = blk};
            worklist.push_back(item);
            marked_jmp.insert(blk);
        }
    }

    

    //print_info(marked_i, marked_phi, marked_jmp, marked_blk, def, def_phi, fn, rdom, ridom, rdf);

    while(!worklist.empty()) {
        auto item = *worklist.begin();
        worklist.erase(worklist.begin());

        if (item.type_st == ins) {
            Ref left = item.ins_st->arg[0];
            Ref right = item.ins_st->arg[1];

            std::vector<Ref> uses_in_rhs = {left, right};

            for (auto u : uses_in_rhs) {
                
                if (u.val < Tmp0) {
                    continue;
                }

                for (auto id : def[u]) {
                    if (marked_i.find(id) == marked_i.end()) {
                        marked_i.insert(id);
                        WorklistItem itt = {.type_st = ins, .blk_st = ins_to_blk[id], .ins_st = id};
                        worklist.push_back(itt);
                    }
                }

                for (auto id : def_phi[u]) {
                    if (marked_phi.find(id) == marked_phi.end()) { 
                        marked_phi.insert(id);
                        WorklistItem itt = {.type_st = phi, .blk_st = phi_to_blk[id], .phi_st = id};
                        worklist.push_back(itt);
                    }
                }
            }
        }

        if (item.type_st == phi) {
            auto args = item.phi_st->arg;
            for (int jj = 0; jj < item.phi_st->narg; jj++) {
                Ref u = item.phi_st->arg[jj];

                if (u.val >= Tmp0) {
                    for (auto id : def[u]) {
                        if (marked_i.find(id) == marked_i.end()) {
                            marked_i.insert(id);
                            WorklistItem itt = {.type_st = ins, .blk_st = ins_to_blk[id], .ins_st = id};
                            worklist.push_back(itt);
                        }
                    }

                    for (auto id : def_phi[u]) {
                        if (marked_phi.find(id) == marked_phi.end()) {
                            marked_phi.insert(id);
                            WorklistItem itt = {.type_st = phi, .blk_st = phi_to_blk[id], .phi_st = id};
                            worklist.push_back(itt);
                        }
                    }
                }

            }

        }

        if (item.type_st == jmp) {
            Ref u = item.blk_st->jmp.arg;
            if (u.val >= Tmp0) {
                for (auto id : def[u]) {
                    if (marked_i.find(id) == marked_i.end()) {
                        marked_i.insert(id);
                        WorklistItem itt = {.type_st = ins, .blk_st = ins_to_blk[id], .ins_st = id};
                        worklist.push_back(itt);
                    }
                }

                for (auto id : def_phi[u]) {
                    if (marked_phi.find(id) == marked_phi.end()) {
                        marked_phi.insert(id);
                        WorklistItem itt = {.type_st = phi, .blk_st = phi_to_blk[id], .phi_st = id};
                        worklist.push_back(itt);
                    }
                }
            }

        }

        //std::cout << item.blk_st->name << " ff \n";
        for (auto b : rdf[item.blk_st]) {
            //std::cout << item.blk_st->name << " " << b->name << " in rdf\n";
            
            if (marked_jmp.find(b) == marked_jmp.end()) {    
                WorklistItem itt = {.type_st = jmp, .blk_st = b};
                worklist.push_back(itt);
                marked_jmp.insert(b);
            }

            if (item.type_st == ins) {
                if (marked_i.find(item.ins_st) != marked_i.end()) {
                    marked_jmp.insert(b);
                }
            }

            if (item.type_st == phi) {
                if (marked_phi.find(item.phi_st) != marked_phi.end()) {
                    marked_jmp.insert(b);
                }
            }

        }
        

    }

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        int cnt = 0;
        for (Ins *i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            if (marked_i.find(i) != marked_i.end()) {
                cnt++;
            }
        }

        for (Phi *i = blk->phi; i; i = i->link) {
            if (marked_phi.find(i) != marked_phi.end()) {
                cnt++;
            }
        }

        if (cnt > 0 || (marked_jmp.find(blk) != marked_jmp.end())) {
            marked_blk.insert(blk);        
        }
    }

    //print_info(marked_i, marked_phi, marked_jmp, marked_blk, def, def_phi, fn, rdom, ridom, rdf);
    sweep(fn, marked_i, marked_phi, marked_jmp, marked_blk, ridom);
}

static void readfn (Fn *fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    
    mark_sweep(fn);

    fillpreds(fn); // Either call this, or keep track of preds manually when rewriting branches.
    fillrpo(fn); // As a side effect, fillrpo removes any unreachable blocks.
    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}