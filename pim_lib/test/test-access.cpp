#include <fcntl.h>
#include <sys/mman.h>
#include <cstdio>
#include <cassert>

#include "pim_ops.hpp"

struct RepeatData {

    ptr_t r[8];
    
    RepeatData(unsigned char c) {
        r[0] = c;
        for (int i = 1; i < 8; i++) {
            r[i] = (r[i - 1] << 8) | c;
        }
    }
};

struct Lob {    
    
    ptr_t data[515];
 
    Lob(ptr_t val) {
        for (int i = 0; i < sizeof(data) / sizeof(ptr_t); i++) {
            data[i] = val;
        }
    }
};

int main() {
    using node_t = pim_list_node<RepeatData>;
    
    RepeatData rd(0xEE);

    node_t* n1 = new node_t(0xAA);
    node_t* n2 = new node_t(0xBB);
    node_t* n3 = new node_t(0xCC);
    node_t* n4 = new node_t(0xDD);
    
    n1->next(n2);
    n2->next(n3);
    n3->next(n4);

    assert(pim_find(n1, n4->data()) == n4);
    assert(pim_find(n1, rd) == nullptr);    

    delete n1;
    delete n2;
    delete n3;
    delete n4;

    using lod_t = pim_list_node<Lob>;

    Lob ld(0xEE);

    auto* l1 = new lod_t(0xAA);
    auto* l2 = new lod_t(0xBB);
    auto* l3 = new lod_t(0xCC);
    auto* l4 = new lod_t(0xDD);
    
    l1->next(l2);
    l2->next(l3);
    l3->next(l4);
    
    assert(pim_find(l1, l4->data()) == l4);
    assert(pim_find(l1, ld) == nullptr);
}

