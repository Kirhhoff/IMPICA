#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "pimmem.h" 

void test_alloc_align() {
    size_t request_size[] = {
        1, 2, 3, 4, 5, 8, 9, 16, 17, 24, 25, 32
    };

    for (int i = 0; i < sizeof(request_size) / sizeof(size_t); i++) {
        CU_ASSERT_NOT_EQUAL(((vaddr_t)pim_alloc(request_size[i])), 0);   
        CU_ASSERT(((vaddr_t)pim_alloc(request_size[i])) % 8 == 0);   
    }
}

void test_alloc_single() {
    typedef long T;
    T *p = (T *)pim_alloc(sizeof(T));
    
    CU_ASSERT_PTR_NOT_NULL_FATAL(p);
    CU_ASSERT_PTR_NOT_NULL_FATAL(half.head);
    
    vma_t *vma = half.head; 
    CU_ASSERT_NOT_EQUAL(vma->va_begin, 0);
    CU_ASSERT_PTR_NULL(vma->next);

    page_t *page;
    free_store_t *free_store;
    for (int i = 0; i < N_PAGE_PER_VMA; i++) {
        page = &vma->pages[i];
        
        CU_ASSERT_PTR_NOT_NULL_FATAL(page->free_store.next);
        CU_ASSERT_PTR_EQUAL(page->free_store.next, page->free_store.prev);
        CU_ASSERT_PTR_NOT_EQUAL_FATAL(page->free_store.next, &page->free_store);

        free_store = list2freestore(page->free_store.next);
        CU_ASSERT_PTR_EQUAL(free_store->list_head.prev, &page->free_store);
        CU_ASSERT_PTR_EQUAL(free_store->list_head.next, &page->free_store);
        
        if (i != 0) {
            CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);
            CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + i * PAGE_SIZE);
        }
    }

    page = &vma->pages[0];
    free_store = list2freestore(page->free_store.next);
    
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - pimsizeof(T));
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + pimsizeof(T))

    pim_free(p, sizeof(T)); 
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);

    void *p1, *p2, *p3, *p4, *p5;
    list_head_t *head;
    size_t unit;
    head = &page->free_store;

    // test case 1:
    p1 = pim_alloc(rawsize(PAGE_SIZE));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p1, rawsize(PAGE_SIZE));
    CU_ASSERT_EQUAL(list_size(head), 1);
    
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);

    // test case 2:
    unit = PAGE_SIZE / 4;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - 2 * unit);

    p3 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);

    pim_free(p3, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - 2 * unit);

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - 2 * unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);

    // test case 3:
    unit = PAGE_SIZE / 2;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);

    // test case 4:
    unit = PAGE_SIZE / 4;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - 2 * unit);

    p3 = pim_alloc(rawsize(2 * unit));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next); 
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p3, rawsize(2 * unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, 2 * unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);

    // test case 5:
    unit = PAGE_SIZE / 2;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE);

    // test case 6:
    unit = PAGE_SIZE / 8;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, 7 * unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, 6 * unit);
    
    p3 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, 5 * unit);
    
    p4 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 4 * unit);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);

    p5 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 5 * unit);
    CU_ASSERT_EQUAL(free_store->size, 3 * unit);

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);
    
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 5 * unit);
    CU_ASSERT_EQUAL(free_store->size, 3 * unit);

    pim_free(p5, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 4 * unit);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);

    pim_free(p3, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 3);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 4 * unit);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, 3 * unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 4 * unit);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);

    pim_free(p4, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, 8 * unit);

    // test case 7:
    unit = PAGE_SIZE / 4;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, 3 * unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, 2 * unit);
    
    p3 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);
    
    p4 = pim_alloc(rawsize(unit));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p4, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, 2 * unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p3, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);

    // test case 8:
    unit = PAGE_SIZE / 4;
    p1 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + unit);
    CU_ASSERT_EQUAL(free_store->size, 3 * unit);

    p2 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, 2 * unit);
    
    p3 = pim_alloc(rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);
    
    p4 = pim_alloc(rawsize(unit));
    CU_ASSERT_TRUE(list_empty(head));

    pim_free(p1, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p4, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 3 * unit);
    CU_ASSERT_EQUAL(free_store->size, unit);

    pim_free(p3, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 2);

    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, unit);

    free_store = list2freestore(head->next->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin + 2 * unit);
    CU_ASSERT_EQUAL(free_store->size, 2 * unit);

    pim_free(p2, rawsize(unit));
    CU_ASSERT_EQUAL(list_size(head), 1);
    free_store = list2freestore(head->next);
    CU_ASSERT_EQUAL(free_store->begin, vma->va_begin);
    CU_ASSERT_EQUAL(free_store->size, 4 * unit);
}

int reset_alloc() {
    
    return 0;
}

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    CU_pSuite pSuite = CU_add_suite("test_alloc", NULL, reset_alloc);
    // CU_ADD_TEST(pSuite, test_alloc_align);
    CU_ADD_TEST(pSuite, test_alloc_single);


    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list()); 
    printf("\n\n");
    CU_cleanup_registry();
    printf("\n");
}