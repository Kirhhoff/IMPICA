#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "pimmem.h" 

void test_alloc_align() {
    size_t request_size[] = {
        1, 2, 3, 4, 5, 8, 9, 16, 17, 24, 25, 32
    };

    for (int i = 0; i < sizeof(request_size) / sizeof(size_t); i++) {
        CU_ASSERT(((vaddr_t)pim_alloc(request_size[i])) % 8 == 0);   
    }
}

void test_alloc_single() {
    typedef long T;
    T *p = (T *)pim_alloc(sizeof(T));

    CU_ASSERT_EQUAL(half.max, PAGE_SIZE - pimsizeof(T));    
    CU_ASSERT_PTR_NOT_NULL_FATAL(half.head);
    
    vma_t *vma = half.head; 
    CU_ASSERT_EQUAL(vma->max, PAGE_SIZE - pimsizeof(T));    
    CU_ASSERT_NOT_EQUAL(vma->vpn_begin, 0);
    CU_ASSERT_PTR_NULL(vma->next);

    for (int i = 1; i < N_PAGE_PER_VMA; i++) {
        CU_ASSERT_EQUAL(vma->pages[i].max, 0);
        CU_ASSERT_EQUAL(vma->pages[i].pfn, 0);
        CU_ASSERT_EQUAL(vma->pages[i].head.size, 0);
        CU_ASSERT_PTR_NULL(vma->pages[i].head.next);
    }

    page_t *page = &vma->pages[0];
    CU_ASSERT_EQUAL(page->max, PAGE_SIZE - pimsizeof(T)); 
    CU_ASSERT_NOT_EQUAL(page->pfn, 0);

    free_store_t *free_store = &page->head;
    CU_ASSERT_EQUAL(free_store->size, PAGE_SIZE - pimsizeof(T));
    CU_ASSERT_PTR_NULL(free_store->next);   

    CU_ASSERT_EQUAL(empty.max, 0);    
    CU_ASSERT_PTR_NULL(empty.head);
}

int reset_alloc() {

}

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }
    CU_pSuite pSuite = CU_add_suite("test_alloc", NULL, reset_alloc);
    CU_ADD_TEST(pSuite, test_alloc_align);
    CU_ADD_TEST(pSuite, test_alloc_single);


    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list()); 
    printf("\n\n");
    CU_cleanup_registry();
    printf("\n");
}