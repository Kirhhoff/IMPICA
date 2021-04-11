#ifndef PIM_SYSCALL_H
#define PIM_SYSCALL_H

#include "syscall.hpp"

class system_tag {};

struct system_syscall{
    typedef system_tag type_tag;
};

void* pim_allocate(psize_t size, system_tag tag);
int pim_mlock(void* addr, psize_t len, system_tag tag);
ptr_t va2pa(ptr_t va, system_tag tag);

#endif