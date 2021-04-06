#include "pimmem.h"
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef TEST
#define DEFINE_REGION(name) static region_t name = { \
    .head = NULL_VMA \
};
#else
#define DEFINE_REGION(name) region_t name = { \
    .head = NULL_VMA \
};
#endif

DEFINE_REGION(half);
DEFINE_REGION(empty);

// TODO: implement allocate from free_store
static void *alloc_from_page(page_t *page, size_t size) {
    free_store_t *free_store;
    list_head_t *head;
    void* addr;

    printf("[LOG]: page free_store size: %d\n", list_size(&page->free_store)); 

    head = page->free_store.next;
    while (head != &page->free_store) {
        free_store = list2freestore(head); 
        if (free_store->size >= size) {
            addr = (void *)free_store->begin;
            if (free_store->size > size) {
                free_store->size -= size;
                free_store->begin += size;
            } else {
                list_delete(&free_store->list_head);
                free(free_store);
            }
            printf("[LOG]: allocate p = [0x%lx] size = [0x%lx]\n", (vaddr_t)addr, size);
            return addr;
        }
        head = head->next;
    }

    return NULL;
}

static void *alloc_from_vma(vma_t *vma, size_t size) {
    page_t *page;
    void *addr;

    page = &vma->pages[0];
    for (int i = 0; i < N_PAGE_PER_VMA; i++, page++) {
        addr = alloc_from_page(page, size);
        if (addr) {
            return addr;
        }
    }    
    
    return NULL;
}

static void *try_alloc_from_half(size_t size) {
    vma_t *vma;
    void *addr;

    printf("[LOG]: try to allocate from half\n");
    printf("[LOG]: half head = [0x%lx]\n", (vaddr_t)half.head);

    vma = half.head;
    while (vma) {
        addr = alloc_from_vma(vma, size);
        if (addr) {
            return addr;
        } else {
            vma = vma->next;
        }
    }

    return NULL;
}

// TODO: allocate from empty
static void *try_alloc_from_empty(size_t size) {
    return NULL;
}

// TODO: setup vma pa
static int setup_vma(vma_t *vma) {
    page_t *page;
    free_store_t *free_store;

    page = &vma->pages[0];
    for (int i = 0; i < N_PAGE_PER_VMA; i++, page++) {
        // page->pa_begin = ;

        free_store = (free_store_t *)malloc(sizeof(free_store_t));
        if (!free_store) {
            // TODO: roll back
            printf("[LOG]: malloc free_store failed!\n");          
            return -1;
        }
        free_store->size = PAGE_SIZE;
        free_store->begin = vma->va_begin + i * PAGE_SIZE;
        list_init(&page->free_store);
        list_insert_after(&free_store->list_head, &page->free_store); 
    }

    return 0;
}

static vma_t *alloc_vma() {
    void *vma_begin;
    vma_t *vma;
    int err;

    vma_begin = mmap(0, VMA_SIZE, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);
    
    if (MAP_FAILED == vma_begin) {
        printf("[LOG]: mmap failed!\n");
        printf("[LOG]: %s\n", strerror(errno));
        return NULL;
    }

    printf("[LOG]: mmap start = [0x%lx]\n", (vaddr_t)vma_begin);

    if (mlock(vma_begin, VMA_SIZE) < 0) {
        printf("[LOG]: mlock failed!\n");
        return NULL;
    }

    vma = (vma_t *)malloc(sizeof(vma_t));

    if (!vma) {
        printf("[LOG]: malloc vma failed!\n");
        return NULL;
    }    

    vma->va_begin = (vaddr_t)vma_begin;
    err = setup_vma(vma);
    if (err < 0) {
        return NULL;
    }

    return vma;
}

static void insert_half_vma(vma_t *vma) {
    vma->next = half.head;
    half.head = vma;
}

static void *try_alloc_from_mmap(size_t size) {
    vma_t *vma;
    
    printf("[LOG]: try to allocate size = [0x%lx] from mmap\n", size);

    vma = alloc_vma();

    if (!vma) {
        return NULL;
    }

    insert_half_vma(vma);

    return try_alloc_from_half(size);
}

void *pim_alloc(size_t size) {
    void *addr;

    size = pimsize(size);

    addr = try_alloc_from_half(size);
    if (addr != NULL) {
        goto success;
    }

    printf("[LOG]: try to allocate size = [0x%lx] from empty\n", size);
    addr = try_alloc_from_empty(size); 
    if (addr != NULL) {
        goto success;
    }

    addr = try_alloc_from_mmap(size);
    if (addr != NULL) {
        goto success;
    }

    return NULL;

success:
    return ((pim_node_t *)addr)->data;
}

static vma_t *get_vma(void *p, size_t size) {
    vma_t *vma;

    vma = half.head;
    while (vma) {
        if (vma->va_begin <= (vaddr_t)p && (vaddr_t)p + size <= vma->va_begin + PAGE_SIZE) {
            return vma;
        }
        vma = vma->next;
    }

    return NULL;
}

void pim_free(void *p, size_t size) {
    vma_t *vma;
    page_t *page;
    list_head_t *head_next;
    free_store_t *prev_free, *next_free, *itr;

    p = container_of(p, pim_node_t, data);
    size = pimsize(size);

    printf("[LOG]: begin free p = [0x%lx] size = [0x%lx]\n", (vaddr_t)p, size);
    vma = get_vma(p, size);
    if (!vma) {
        printf("[LOG]: free p = 0x%lx size = %ld failed!\n", (vaddr_t)p, size);
        return;
    }      

    page = &vma->pages[((vaddr_t)p - vma->va_begin) / PAGE_SIZE];
    prev_free = NULL;
    next_free = NULL;
    head_next = page->free_store.next;
    while (head_next != &page->free_store) {
        itr = list2freestore(head_next);
        if (itr->begin > (vaddr_t)p) {
            next_free = itr;
            break; 
        }
        head_next = head_next->next;   
    }

    // try to merge prev
    if (head_next->prev != &page->free_store) {
        prev_free = list2freestore(head_next->prev);
        if (prev_free->begin + prev_free->size == (vaddr_t)p) {
            prev_free->size += size;
            if (next_free && prev_free->begin + prev_free->size == next_free->begin) {
                prev_free->size += next_free->size;
                list_delete(&next_free->list_head);
                free(next_free);
            }
            return; 
        } 
    }

    // try to merge next
    if (next_free && (vaddr_t)p + size == next_free->begin) {
        next_free->begin = (vaddr_t)p;
        next_free->size += size;   
        return;
    }

    // cannot merge, allocate a new free_store
    prev_free = (free_store_t *)malloc(sizeof(free_store_t));
    prev_free->begin = (vaddr_t)p;
    prev_free->size = size;
    list_insert_before(&prev_free->list_head, head_next);
}