#ifndef PIM_OPS_HPP
#define PIM_OPS_HPP

#include "pim_list_node.hpp"

inline void iowrite64(void* addr, unsigned long val) {
    volatile unsigned long *ptr = (volatile unsigned long *)addr;
    *ptr = val;
}

inline unsigned long ioread64(void* addr) {
    volatile unsigned long *ptr = (volatile unsigned long *)addr;
    return *ptr;
}

inline unsigned int ioread32(void* addr) {
    volatile unsigned int *ptr = (volatile unsigned int *)addr;
    return *ptr;
}

template<typename T, class syscall_type, class page_manager_impl>
pim_list_node<T, syscall_type, page_manager_impl>* 
pim_find(pim_list_node<T, syscall_type, page_manager_impl>* begin,const T& val) {
    using node_t = pim_list_node<T, syscall_type, page_manager_impl>;
    
    auto& pim_reg = PimThreadContext::ctx.pim_reg;
    node_t target(val);

    iowrite64(&pim_reg[Pthis], begin->pthis(0));
    iowrite64(&pim_reg[Ptarget], target.pthis(0));
    iowrite64(&pim_reg[Vthis], reinterpret_cast<unsigned long>(begin));
    iowrite64(&pim_reg[Tsize], node_t::T_size);

    int cnt = 0;
    while (1 != ioread32(&pim_reg[Done])) 
        cnt++;
    
    printf("[LOG:] pim_find poll cnt: %d\n", cnt);
    
    return reinterpret_cast<node_t*>(ioread64(&pim_reg[Vthis]));
}

#endif