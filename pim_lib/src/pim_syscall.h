#ifndef PIM_SYSCALL_H
#define PIM_SYSCALL_H

#include "syscall.hpp"

class system_tag {};

struct system_syscall{
    typedef system_tag type_tag;
};

class PimThreadContext {
private:
    PimThreadContext();

public:
    static const PimThreadContext ctx;
    
    int ptable_fd;
    char* pim_reg;
};

void* pim_malloc(psize_t size, system_tag tag);
void pim_free(void* ptr, system_tag tag);
int pim_mlock(void* addr, psize_t len, system_tag tag);
int pim_munlock(void* addr, psize_t len, system_tag tag);
ptr_t va2pa(ptr_t va, system_tag tag);

#endif