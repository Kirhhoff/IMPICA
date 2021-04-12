#ifndef DEFAULT_PAGE_MANAGER_HPP
#define DEFAULT_PAGE_MANAGER_HPP

#include <unordered_map>
#include <cassert>
#include "pim_syscall.h"
#include "page_manager.hpp"

using std::unordered_map;

template<class syscall_type = system_syscall> 
class default_page_manager : public page_manager<default_page_manager<syscall_type>> {
public:
    static default_page_manager instance;

    ptr_t query_and_lock(ptr_t vaddr);
    void release(ptr_t vaddr);

protected:
    using type_tag = typename syscall_trait<syscall_type>::type_tag;

    int mlock(void* addr, psize_t size) {
        return pim_mlock(addr, size, type_tag());
    }
    
    int munlock(void* addr, psize_t size) {
        return pim_munlock(addr, size, type_tag());
    }

    ptr_t va2pa(ptr_t vaddr) {
        return ::va2pa(vaddr, type_tag());
    }

protected:
    using vaddr_t = ptr_t;
    using paddr_t = ptr_t;
    using ref_cnt_t = psize_t;
    using pa_ref_pair = std::pair<paddr_t, ref_cnt_t>;

    unordered_map<vaddr_t, pa_ref_pair> va2pa_map;
};

template<class syscall_type>
default_page_manager<syscall_type> default_page_manager<syscall_type>::instance;

template<class syscall_type>
ptr_t default_page_manager<syscall_type>::query_and_lock(ptr_t vaddr) {
    paddr_t paddr, offset = vaddr & PAGE_MASK;
    vaddr &= ~PAGE_MASK;

    if (va2pa_map.find(vaddr) != va2pa_map.end()) {
        va2pa_map[vaddr].second++;
        paddr = va2pa_map[vaddr].first;
    } else {
        mlock(reinterpret_cast<void*>(vaddr), PAGE_SIZE);
        paddr = va2pa(vaddr);
        va2pa_map.emplace(vaddr, std::make_pair(paddr, 1));
    }

    return paddr | offset;
}

template<class syscall_type>
void default_page_manager<syscall_type>::release(ptr_t vaddr) {
    vaddr &= ~PAGE_MASK;

    assert(va2pa_map.find(vaddr) != va2pa_map.end() && va2pa_map[vaddr].second > 0);

    if (va2pa_map[vaddr].second > 1) {
        va2pa_map[vaddr].second--;
    } else {
        munlock(reinterpret_cast<void*>(vaddr), PAGE_SIZE);
        va2pa_map.erase(vaddr);
    }
}

#endif