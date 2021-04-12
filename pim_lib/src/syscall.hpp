#ifndef SYSCALL_HPP
#define SYSCALL_HPP

#include "pim_defs.h"

template<typename syscall_type>
struct syscall_trait {
   typedef typename syscall_type::type_tag type_tag;
};

template<typename syscall_tag_type>
void* pim_malloc(psize_t size, syscall_tag_type tag);

template<typename syscall_tag_type>
void pim_free(void* ptr, syscall_tag_type tag); 

template<typename syscall_tag_type>
int pim_mlock(void* addr, psize_t len, syscall_tag_type tag);

template<typename syscall_tage_type>
int pim_munlock(void* addr, psize_t len, syscall_tage_type tag);

template<typename syscall_tag_type>
ptr_t va2pa(ptr_t va, syscall_tag_type tag);

#endif