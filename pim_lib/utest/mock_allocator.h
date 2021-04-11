#include "syscall.hpp"
#include <gmock/gmock.h>

class mock_tag {};

class mock_syscall{
public:
    typedef mock_tag type_tag;
};

void* pim_allocate(psize_t size, mock_tag tag);
int pim_mlock(void* addr, psize_t len, mock_tag tag);
ptr_t va2pa(ptr_t, mock_tag);