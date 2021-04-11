#ifndef DEFAULT_PAGE_MANAGER_HPP
#define DEFAULT_PAGE_MANAGER_HPP

#include "page_manager.hpp"
#include "pim_syscall.h"
#include <cstdio>

template<class syscall_type = system_syscall> 
class default_page_manager : public page_manager<default_page_manager<syscall_type>> {

public:
    static default_page_manager instance;
    ptr_t query_and_lock(ptr_t vaddr);
    void release(ptr_t vaddr);

protected:
    ptr_t va2pa(ptr_t vaddr) {
        typedef typename syscall_trait<syscall_type>::type_tag type_tag;
        return ::va2pa(vaddr, type_tag());
    }
};

template<class syscall_type>
default_page_manager<syscall_type> default_page_manager<syscall_type>::instance;

// TODO
template<class syscall_type>
ptr_t default_page_manager<syscall_type>::query_and_lock(ptr_t vaddr) {
    return va2pa(vaddr);    
}

#endif