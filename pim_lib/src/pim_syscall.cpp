#include "pim_syscall.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>

const PimThreadContext PimThreadContext::ctx;

PimThreadContext::PimThreadContext() {
    pid_t pid;
    char filename[64];
    error_t err;
    int pim_fd;

    pid = getpid();
    if (pid < 0) {
        printf("Fail to get pid: %s\n", strerror(pid));
        return;   
    }

    err = snprintf(filename, sizeof(filename), "/proc/%d/pagemap", pid);
    if (err < 0) {
        printf("Fail to generate filename: %s\n", strerror(err));
        return;
    }

    ptable_fd = open(filename, O_RDONLY);
    if (ptable_fd < 0) {
        printf("Fail to open page table file: %s\n", strerror(ptable_fd));
        return;
    }

    pim_fd = open("/dev/pimbt", O_RDWR);
    
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
}

void* pim_malloc(psize_t size, system_tag tag){
    void* addr = malloc(size);
    printf("malloc at %p, size = 0x%lx\n", addr, size);
    return addr;
}

void pim_free(void* ptr, system_tag tag) {
    printf("free at %p\n", ptr);
    free(ptr);
}

int pim_mlock(void* addr, psize_t len, system_tag tag) {
    int ret = mlock(addr, len);
#ifdef PIM_SYSCALL_DEBUG

    printf("[LOG:] mlock at [%p], size = [0x%lx]\n", addr, len);

#endif
    return ret;
}

int pim_munlock(void* addr, psize_t len, system_tag tag) {
    int ret = munlock(addr ,len);
#ifdef PIM_SYSCALL_DEBUG

    printf("[LOG:] munlock at [%p], size = [0x%lx]\n", addr, len);

#endif
    return ret;
}

ptr_t va2pa(ptr_t addr, system_tag tag) {
    ptr_t entry, pa;
    error_t err;

    err = pread(PimThreadContext::ctx.ptable_fd,
        &entry, sizeof(entry),
        (addr >> PAGE_SHIFT) * sizeof(entry));

    if (err < sizeof(entry)) {
        printf("Fail to read page table entry!\n");
    }

    pa = entry << PAGE_SHIFT;
#ifdef PIM_SYSCALL_DEBUG

    printf("[LOG:] va2pa: [0x%lx -> 0x%lx]\n", addr, pa);

#endif
    return pa;
}