#ifndef PIM_LIST_NODE_HPP
#define PIM_LIST_NODE_HPP

#include <new>
#include "pim_defs.h"
#include "pim_syscall.h"
#include "default_page_manager.hpp"
#include <cstdio>

template<typename T,
    class syscall_type = system_syscall,
    class page_manager_impl = default_page_manager<system_syscall>>
class pim_list_node {

protected:
    
    static constexpr psize_t T_size = sizeof(T);
    static constexpr psize_t chunk_size = sizeof(chunk_t);
    static constexpr psize_t n_T_ptr = (T_size + PTR_SIZE - 1) / PTR_SIZE;
    static constexpr psize_t n_pim_link_ptr = 2;
    static constexpr psize_t n_est_meta_ptr = 1 + round_up(n_pim_link_ptr + n_T_ptr - 1, N_PTR_PER_PAGE) / N_PTR_PER_PAGE;
    static constexpr psize_t n_pim_meta_ptr = 1 + round_up(n_pim_link_ptr + n_T_ptr + n_est_meta_ptr - 1, N_PTR_PER_PAGE) / N_PTR_PER_PAGE;
    static constexpr psize_t n_chunk = n_pim_meta_ptr + n_pim_link_ptr + n_T_ptr;
    static constexpr psize_t pim_size = n_chunk * chunk_size;

    using node = pim_list_node<T, syscall_type>;

    static page_manager<page_manager_impl>& pm;

    enum {
        IDX_META = 0,
        IDX_PNEXT = n_pim_meta_ptr,
        IDX_VNEXT,
        IDX_DATA_BEGIN
    };

    chunk_t mem[n_chunk];    

protected:

    ptr_t& pthis(psize_t idx) {
        return mem[IDX_META + idx];
    }

    ptr_t& pnext() {
        return mem[IDX_PNEXT];
    }

    ptr_t& vnext() {
        return mem[IDX_VNEXT];
    }

    static void* operator new(psize_t count) {
        typedef typename syscall_trait<syscall_type>::type_tag type_tag;
        ptr_t* mem = static_cast<ptr_t*>(pim_allocate(count, type_tag()));
        mem[0] = reinterpret_cast<ptr_t>(mem);
        return mem;
    }

public:

    // TODO: deal with pthis
    pim_list_node(){
        ptr_t vthis = pthis(0), vend = pthis(0) + pim_size - chunk_size;
        psize_t expage = (vend >> PAGE_SHIFT) - (vthis >> PAGE_SHIFT);
        printf("from pm : %lx\n", pm.query_and_lock(0)); 
        if(expage == 0) {
            // pthis(0) = ;
        } else {
            // ptr_t idx = 1;
            // ptr_t vpn = (vthis >> PAGE_SIZE + 1);
            // while (expage-- > 0) {
            //     pthis(idx++) = ; // q(vpn << PAGE_SIZE)
            //     vpn++;
            // }
        }
        pnext() = 0;
        vnext() = 0;
    }

    T& data(){
        return *(reinterpret_cast<T*>(&mem[IDX_DATA_BEGIN]));
    }

    // TODO
    template<class E>
    void next(pim_list_node<E>* __next){
    
    }

    // TODO
    node* next(){
        return nullptr;
    }    

    // TODO
    template<class E>
    pim_list_node<E>* next_as(){
        return nullptr;
    }
};

template<typename T, class syscall_type, class page_manager_impl>
page_manager<page_manager_impl>& pim_list_node<T, syscall_type, page_manager_impl>::pm = page_manager_impl::instance;

#endif