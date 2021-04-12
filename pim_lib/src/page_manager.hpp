#ifndef PAGE_MANAGER_HPP
#define PAGE_MANAGER_HPP

#include "pim_defs.h"

template<typename page_manager_impl>
class page_manager {
public:
    ptr_t query_and_lock(ptr_t vaddr){
        return static_cast<page_manager_impl*>(this)->query_and_lock(vaddr);
    }

    void release(ptr_t vaddr) {
        static_cast<page_manager_impl*>(this)->release(vaddr);
    }    
};


#endif