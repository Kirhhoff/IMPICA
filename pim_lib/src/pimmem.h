#ifndef PIM_MEM_H
#define PIM_MEM_H

#include <stddef.h>
#include "list_head.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1 << (PAGE_SHIFT))
#define PAGE_MASK ((PAGE_SIZE) - 1)
#define N_PAGE_PER_VMA 8
#define VMA_SIZE (N_PAGE_PER_VMA * PAGE_SIZE)

typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;
typedef unsigned long chunk_t;

typedef struct free_store {
    size_t size;
    vaddr_t begin;
    list_head_t list_head;
} free_store_t;

typedef struct page_struct {
    paddr_t pa_begin;
    list_head_t free_store;
} page_t;

typedef struct vma_struct{
    vaddr_t va_begin;
    page_t pages[N_PAGE_PER_VMA];
    struct vma_struct *next;
} vma_t;

typedef struct region_struct {
    vma_t *head;
} region_t;

typedef struct pim_node {
    paddr_t pnext;
    vaddr_t vnext;
    paddr_t pthis;
    chunk_t data[];
} pim_node_t;

#define NULL_VMA ((vma_t *)NULL)

#define PIM_PTR_SIZE (8)
#define N_HEAD_PTR (3)
#define PIM_HEAD_SIZE (N_HEAD_PTR * PIM_PTR_SIZE)

#define pimsize(size) (PIM_HEAD_SIZE + ((size) + PIM_PTR_SIZE - 1) / PIM_PTR_SIZE * PIM_PTR_SIZE)
#define pimsizeof(T) (pimsize(sizeof(T)))
#define rawsize(size) ((size) - PIM_HEAD_SIZE)

#define vpn(va) ((va) >> PAGE_SHIFT)
#define va(vpn) ((vaddr_t)((vpn) << PAGE_SHIFT))

#define pimaddr(raw_addr) ((void *)(((vaddr_t)raw_addr) - PIM_HEAD_SIZE))
#define rawaddr(pim_addr) ((void *)(((vaddr_t)pim_addr) + PIM_HEAD_SIZE))

#define member_offset(type, member) ((vaddr_t)(&(((type *)0)->member)))
#define container_of(ptr, type, member) ((type *)(((vaddr_t)ptr) - member_offset(type, member)))

#define list2freestore(ptr) (container_of(ptr, free_store_t, list_head))

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