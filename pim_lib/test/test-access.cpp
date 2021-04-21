#include <fcntl.h>
#include <sys/mman.h>
#include <cstdio>
#include <cassert>

#include "pim_list_node.hpp"

#define PIM_MAP_FAILED ((void*)-1)

static const int Done           = 0x01C;
static const int Pthis			= 0x038;
static const int Ptarget		= 0x040;
static const int Vthis			= 0x048;
static const int Tsize			= 0x050;

static void iowrite64(void* addr, unsigned long val) {
    volatile unsigned long *ptr = (volatile unsigned long *)addr;
    *ptr = val;
}

static unsigned long ioread64(void* addr) {
    volatile unsigned long *ptr = (volatile unsigned long *)addr;
    return *ptr;
}

static unsigned int ioread32(void* addr) {
    volatile unsigned int *ptr = (volatile unsigned int *)addr;
    return *ptr;
}

char* pim_reg;

template<typename T, class syscall_type, class page_manager_impl>
pim_list_node<T, syscall_type, page_manager_impl>* 
pim_find(pim_list_node<T, syscall_type, page_manager_impl>* begin, T& val) {
    using node_t = pim_list_node<T, syscall_type, page_manager_impl>;
    
    node_t target(val);
    printf("before 4 iowrite\n");
    iowrite64(&pim_reg[Pthis], begin->pthis(0));
    iowrite64(&pim_reg[Ptarget], target.pthis(0));
    iowrite64(&pim_reg[Vthis], reinterpret_cast<unsigned long>(begin));
    iowrite64(&pim_reg[Tsize], node_t::T_size);

    printf("before cnt\n");
    int cnt = 1000;
    while (cnt-- > 0 && 1 != ioread32(&pim_reg[Done]));
    printf("cnt = %d\n", cnt);
    if (cnt == 0) {
        printf("Query Done Timeout!\n");
    }
    printf("before ret\n");      
    return reinterpret_cast<node_t*>(ioread64(&pim_reg[Vthis]));
}

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
    int pim_fd = open("/dev/pimbt", O_RDWR);
    
    if (pim_fd < 0) {
        printf("Fail to open pim device.\n");
    } 

    pim_reg = (char *)mmap(0, 0x1000,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        pim_fd, 0);
    
    if (pim_reg == PIM_MAP_FAILED) {
        printf("Fail to map pim registers.\n");
    }

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

