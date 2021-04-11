#include "pim_syscall.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

void* pim_allocate(psize_t size, system_tag tag){
    return malloc(size);
}
ptr_t va2pa(ptr_t addr, system_tag tag) {
    pid_t pid;
    char filename[64];
    int fd;
    ptr_t off;
    uint64_t entry;

    pid = getpid();
    snprintf(filename, sizeof(filename), "/proc/%d/pagemap", pid);
    fd = open(filename, O_RDONLY);
    
    addr = (addr & ~(PAGE_SIZE - 1));
    off = addr / PAGE_SIZE * 8;
    pread(fd, &entry, sizeof(entry), off);

    return entry << PAGE_SHIFT;
}
