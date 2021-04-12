#include <gtest/gtest.h>
#include "pim_list_node.hpp"
#include "mock_syscall.h"

constexpr ptr_t va2pa_offset = 0x100000;

class fake_page_manager : public page_manager<fake_page_manager> {
public:
    ptr_t query_and_lock(ptr_t vaddr){
        return vaddr + va2pa_offset;
    }

    void release(ptr_t vaddr) {

    }
};

template<typename T>
class mnode : public pim_list_node<T, mock_syscall, fake_page_manager> {
public:
    using Base = pim_list_node<T, mock_syscall, fake_page_manager>;
    constexpr static psize_t T_size = Base::T_size;
    ptr_t pnext() { return Base::pnext(); }
};

void* pim_malloc(psize_t size, mock_tag tag) {

}

void pim_free(void* ptr, mock_tag tag) {

}

ptr_t pa2va(ptr_t paddr) {
    return paddr - va2pa_offset;
}

ptr_t min(ptr_t v1, ptr_t v2) {
    return v1 < v2 ? v1 : v2;
}

template<typename T>
mnode<T>* mock_pfind(mnode<T>* nodep, T& val) {
    mnode<T>* target = new mnode<T>(T);
    bool cross = reinterpret_cast<ptr_t>(nodep) & PIM_CROSS_PAGE;
    ptr_t size = mnode<T>::T_size;
    while (nodep) {
        ptr_t pstart = nodep & ~PIM_CROSS_PAGE; 
        ptr_t readlen;
        do {
            readlen = min(size, PAGE_SIZE - (pstart & PAGE_MASK));
            if(memcmp() != 0) {
                break;
            }
            size -= readlen;
            ptr_t addr = 
        } while (cross);
        
        cross = reinterpret_cast<ptr_t>(nodep) & PIM_CROSS_PAGE;
        while (cross) {
            
            /* code */
        }

        nodep = reinterpret_cast<mnode<T>*>(nodep->pnext());
    }
    return nullptr;
}

ptr_t mock_pim_device_find(ptr_t pstart, ptr_t ptarget, psize_t size, psize_t header_size) {
    char buff[PAGE_SIZE];
    char cbuff[PAGE_SIZE];
    psize_t header_size_backup = header_size;
    //read in target
    while (1) {
        ptr_t pthisstart = pstart & ~PIM_CROSS_PAGE;
        ptr_t pos = pthisstart;
        ptr_t readlen;
        do {
            readlen = min(size, PAGE_SIZE - (pos & PAGE_MASK));
            memcpy(cbuff, (void*)pa2va(pos), readlen);
            if (memcmp(&buff[header_size], &cbuff[header_size], readlen - header_size) != 0) {
                break;
            }
            header_size = 0;
            size -= readlen;
        } while(1);
    }
    return 0;
}

TEST(PimListNodeTests, Linking) {

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}