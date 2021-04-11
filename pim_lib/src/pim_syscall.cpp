#include "pim_syscall.h"
#include <cstdlib>

void* pim_allocate(psize_t size, system_tag tag){
    return malloc(size);
}