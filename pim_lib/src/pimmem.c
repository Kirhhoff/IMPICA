#include "pimmem.h"
#include <unistd.h>
#include <sys/mman.h>

#ifndef TEST
#define DEFINE_REGION(name) static region_t name = { \
    .max = 0, \
    .head = NULL_VMA \
};
#else
#define DEFINE_REGION(name) region_t name = { \
    .max = 0, \
    .head = NULL_VMA \
};
#endif

DEFINE_REGION(half);
DEFINE_REGION(empty);

static void* alloc_from_half() {

}

static void* alloc_from_empty() {

}

static void* alloc_from_mmap() {
    mmap(0, VMA_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS)    
    return (void *)1;
}

void* pim_alloc(size_t size) {
    size = pimsize(size);
    if (half.max < size) {
        if (NULL_VMA != empty.head) {
            return alloc_from_empty();
        } else {
            return alloc_from_mmap();
        }
    } else {
        return alloc_from_half();
    }
}