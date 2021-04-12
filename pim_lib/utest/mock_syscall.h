#ifndef MOCK_SYSCALL_H
#define MOCK_SYSCALL_H

#include "syscall.hpp"

class mock_tag {};

class mock_syscall{
public:
    typedef mock_tag type_tag;
};

void* pim_malloc(psize_t size, mock_tag tag);
void pim_free(void* ptr, mock_tag tag);
int pim_mlock(void* addr, psize_t len, mock_tag tag);
int pim_munlock(void* addr, psize_t len, mock_tag tag);
ptr_t va2pa(ptr_t, mock_tag);

#endif