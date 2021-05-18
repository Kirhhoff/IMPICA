#ifndef PIM_LIST_NODE_HPP
#define PIM_LIST_NODE_HPP

#include <new>
#include "default_page_manager.hpp"

template<typename T, class syscall_type, class page_manager_impl>
class pim_list_node;

template<typename T, class syscall_type, class page_manager_impl>
pim_list_node<T, syscall_type, page_manager_impl>* 
pim_find(pim_list_node<T, syscall_type, page_manager_impl>* begin, const T& val);

template<typename T,
    class syscall_type = system_syscall,
    class page_manager_impl = default_page_manager<syscall_type>>
class pim_list_node {

    friend pim_list_node* pim_find<T, syscall_type, page_manager_impl>(pim_list_node* begin, const T& val);
    

protected:
    static constexpr psize_t T_size = sizeof(T);
    static constexpr psize_t chunk_size = sizeof(chunk_t);
    static constexpr psize_t n_T_ptr = (T_size + PTR_SIZE - 1) / PTR_SIZE;
    static constexpr psize_t n_pim_link_ptr = 2;
    static constexpr psize_t n_est_meta_ptr = 1 + round_up(n_pim_link_ptr + n_T_ptr - 1, N_PTR_PER_PAGE) / N_PTR_PER_PAGE;
    static constexpr psize_t n_pim_meta_ptr = 1 + round_up(n_pim_link_ptr + n_T_ptr + n_est_meta_ptr - 1, N_PTR_PER_PAGE) / N_PTR_PER_PAGE;
    static constexpr psize_t n_chunk = n_pim_meta_ptr + n_pim_link_ptr + n_T_ptr;
    static constexpr psize_t pim_size = n_chunk * chunk_size;

    using node = pim_list_node<T, syscall_type, page_manager_impl>;

    static page_manager<page_manager_impl>& pm;

    enum {
        IDX_META = 0,
        IDX_PNEXT = n_pim_meta_ptr - 1,
        IDX_VNEXT,
        IDX_PTHIS0,
        IDX_DATA_BEGIN
    };

    chunk_t mem[n_chunk];    

protected:
    ptr_t& pthis(psize_t idx) {
        assert(idx < n_pim_meta_ptr);
        return idx == 0 ? mem[IDX_PTHIS0] : mem[IDX_META + idx - 1];
    }

    ptr_t& pnext() {
        return mem[IDX_PNEXT];
    }

    ptr_t& vnext() {
        return mem[IDX_VNEXT];
    }

public:
    template<typename... Args>
    pim_list_node(Args&&... args);

    ~pim_list_node();

    typedef typename syscall_trait<syscall_type>::type_tag type_tag;
    static void* operator new(psize_t count) {
        return pim_malloc(count, type_tag());
    }

    static void operator delete(void* ptr) {
        pim_free(ptr, type_tag());
    }
    
    T& data(){
        return *(reinterpret_cast<T*>(&mem[IDX_DATA_BEGIN]));
    }

    template<class E, class Esyscall_type, class Epage_manager_impl>
    void next(pim_list_node<E, Esyscall_type, Epage_manager_impl>* __next){
        if (__next != nullptr) {
            pnext() = reinterpret_cast<ptr_t>(__next->pthis(0));
            vnext() = reinterpret_cast<ptr_t>(__next);
        } else {
            clear_next();
        }
    }

    void clear_next() {
        pnext() = 0;
        vnext() = 0;   
    }

    node* next(){
        return reinterpret_cast<node*>(vnext());
    }    

    // TODO
    template<class E>
    pim_list_node<E>* next_as(){
        return nullptr;
    }
};

template<typename T, class syscall_type, class page_manager_impl>
page_manager<page_manager_impl>& pim_list_node<T, syscall_type, page_manager_impl>::pm = page_manager_impl::instance;

template<typename T, class syscall_type, class page_manager_impl>
template<typename... Args>
pim_list_node<T, syscall_type, page_manager_impl>::pim_list_node(Args&&... args){
    ptr_t vthis = reinterpret_cast<ptr_t>(mem);
    ptr_t vend = vthis + pim_size - chunk_size;
    
    // TODO: constexpr (n_pim_meta_ptr == 2)
    if (n_pim_meta_ptr == 2) {
        vend &= ~PAGE_MASK;
        if ((vthis & ~PAGE_MASK) == vend) {
            pthis(0) = pm.query_and_lock(vthis);
            pthis(1) = PIM_META_END;
        } else {
            pthis(0) = pm.query_and_lock(vthis) | PIM_CROSS_PAGE;
            pthis(1) = pm.query_and_lock(vend) | PIM_META_END;
        }
    } else {
        psize_t idx = 0, expage = (vend >> PAGE_SHIFT) - (vthis >> PAGE_SHIFT);
        pthis(n_est_meta_ptr - 1) = 0;
        while (idx < expage) {
            pthis(idx++) = pm.query_and_lock(vthis) | PIM_CROSS_PAGE;
            vthis = ((vthis >> PAGE_SHIFT) + 1) << PAGE_SHIFT;  
        }
        pthis(idx++) = pm.query_and_lock(vthis);
        pthis(n_est_meta_ptr - 1) |= PIM_META_END;
    }

    pnext() = 0;
    vnext() = 0;
    
    new(reinterpret_cast<T*>(&mem[IDX_DATA_BEGIN])) 
        T(std::forward<Args>(args)...);
}

template<typename T, class syscall_type, class page_manager_impl>
pim_list_node<T, syscall_type, page_manager_impl>::~pim_list_node(){
    ptr_t vthis = reinterpret_cast<ptr_t>(mem);
    ptr_t vend = vthis + pim_size - chunk_size;
    psize_t expage = (vend >> PAGE_SHIFT) - (vthis >> PAGE_SHIFT);

    vthis &= ~PAGE_MASK;
    do {
        pm.release(vthis);
        vthis += PAGE_SIZE;
    } while(expage-- > 0);

    reinterpret_cast<T*>(&mem[IDX_DATA_BEGIN])->~T();
}

#endif