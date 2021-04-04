#ifndef PIM_MEM_H
#define PIM_MEM_H

#include <stddef.h>

#define PAGE_SIZE 4096
#define N_PAGE_PER_VMA 8
#define VMA_SIZE (N_PAGE_PER_VMA * PAGE_SIZE)

typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;

typedef struct free_store {
    size_t size;
    struct free_store *next;
} free_store_t;

typedef struct page_struct {
    size_t max;
    paddr_t pfn;
    free_store_t head;
} page_t;

typedef struct vma_struct{
    size_t max;
    vaddr_t vpn_begin;
    page_t pages[N_PAGE_PER_VMA];
    struct vma_struct *next;
} vma_t;

typedef struct region_struct {
    size_t max;
    vma_t *head;
} region_t;


#define NULL_VMA ((vma_t *)NULL)
#define pimsize(size) (3 * 8 + ((size) + 7) / 8 * 8)
#define pimsizeof(T) (pimsize(sizeof(T)))
#define TEST

#ifdef TEST
extern region_t half;
extern region_t empty;
#endif

void* pim_alloc(size_t size);
void pim_free(void* p, size_t size);
void* pim_next(void* p);
void pim_set_next(void* p, void* next);

void* pim_find(void* begin, void* valp, size_t len);
void* pim_traverse(void* begin);
#endif