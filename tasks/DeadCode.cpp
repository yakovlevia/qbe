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

void get_rdf(Fn *fn, std::map<Blk*, std::set<Blk*>> &rdf) {
    
    // code
    
    for (Blk* blk = fn->start; blk; blk = blk->link) {
        std::set<Blk*> tmp;
        for (int i = 0; i < blk->nfron; i++) {
            tmp.insert(blk->fron[i]);
        }

        rdf[blk] = tmp;
    }
}

void print_info(std::set<Ins*> marked_i, std::set<Phi*> marked_phi, std::set<Blk*> marked_jmp,  std::map<Ref, std::vector<Ins*>> def, std::map<Ref, std::vector<Phi*>> def_phi, Fn *fn){
    std::cout << "-----------Ins-------------------\n";

    for (auto item : marked_i) {
        std::cout << item << " ";
    }

    std::cout << "\n-----------Phi-------------------\n";

    for (auto item : marked_phi) {
        std::cout << item << " ";
    }

    std::cout << "\n-----------jump-------------------\n";

    for (auto item : marked_jmp) {
        std::cout << item->name << " ";
    }

    std::cout << "\n-----------Def-------------------\n";

    for (auto [key, val] : def) {
        auto name = fn->tmp[key.val].name;
        std::cout << name << " : ";
        for (auto q : val) {
            std:: cout << q << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\n-----------Def_Phi------------------\n";

    for (auto [key, val] : def_phi) {
        auto name = fn->tmp[key.val].name;
        std::cout << name << " : ";
        for (auto q : val) {
            std:: cout << q << " ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n-----------Worklist-------------------\n\n";
}

void print_rdf(Fn *fn, std::map<Blk*, std::set<Blk*>> rdf) {
    std::cout << "\n-----------RDF-------------------\n";
    for (auto [key, value] : rdf) {
        auto name = key->name;
        std::cout << name << " : ";

        for (auto q : value) {
            std:: cout << q->name << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n-----------rdf_end----------------\n";
}

static void sweep(Fn *fn, std::set<Ins*> marked_i, std::set<Phi*> marked_phi, std::set<Blk*> marked_jmp) {
    for (Blk* blk = fn->start; blk; blk = blk->link) {
        for (Ins *i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            if (marked_i.find(i) == marked_i.end()) {
                i->op = Onop;
                i->to = R;
                i->arg[0] = R;
                i->arg[1] = R;
            } 
        }

        for (Phi *i = blk->phi; i; i = i->link) {
            if (marked_phi.find(i) == marked_phi.end()) {
                i->to = R;
                for (int j = 0; j < i->narg; j++) {
                    i->arg[j] = R;
                }  
            } 
        }

        if (Jret0 <= blk->jmp.type && blk->jmp.type <= Jretc) {
           
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

    std::map<Ins*, Blk*> ins_to_blk;
    std::map<Phi*, Blk*> phi_to_blk;

    get_rdf(fn, rdf);
    print_rdf(fn, rdf);

    for (Blk* blk = fn->start; blk; blk = blk->link) {
        for (Ins *i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
            ins_to_blk[i] = blk;
            
            if (i->op == Ovacall || i->op == Ocall || (Ostoreb <= i->op && i->op <= Ostored)) {
                WorklistItem item = {.type_st = ins, .blk_st = blk, .ins_st = i};
                marked_i.insert(i);
                worklist.push_back(item);
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

    //print_info(marked_i, marked_phi, marked_jmp, def, def_phi, fn);

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

        if (item.type_st == ins || item.type_st == phi || item.type_st == jmp) {
            for (auto b : rdf[item.blk_st]) {
                if (marked_jmp.find(b) != marked_jmp.find(b)) {
                    WorklistItem item = {.type_st = jmp, .blk_st = b};
                    worklist.push_back(item);
                    marked_jmp.insert(b);
                }
            }
        }
    }

    print_info(marked_i, marked_phi, marked_jmp, def, def_phi, fn);

    sweep(fn, marked_i, marked_phi, marked_jmp);
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
    std::cout << "Go Go go";
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}