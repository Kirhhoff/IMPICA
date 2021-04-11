#include "syscall.hpp"
#include <gmock/gmock.h>

class mock_tag {};

class mock_syscall{
public:
    typedef mock_tag type_tag;
};

void* pim_allocate(psize_t size, mock_tag tag);