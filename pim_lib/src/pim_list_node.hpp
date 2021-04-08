// #include <defs.h>
typedef unsigned long size_t;
typedef size_t uint64_t;
#include <cstdio>
#include "malloc_allocator_wrapper.h"

template<typename T, class allocator_type = MallocAllocatorWrapper>
class pim_list_node {
private:
    static constexpr size_t PimMetaSize = 16;
    static constexpr size_t PimLinkSize = 16;
    static constexpr size_t PimHeadrSize = PimMetaSize + PimLinkSize;
    static constexpr size_t Tsize = sizeof(T);
    static constexpr size_t PimSize =  Tsize;
    static constexpr size_t nchunk = PimSize / 8;
    static constexpr size_t nmetadata = 2;

    static PimAllocator& allocator; 

    typedef uint64_t ptr_t;
    typedef ptr_t chunk_t;
    typedef pim_list_node<T> pim_node;

    chunk_t mem[nchunk];    

    enum {
        IDX_PNEXT = nmetadata,
        IDX_VNEXT,
        IDX_DATA_BEGIN
    };

    ptr_t& pnext() {
        return mem[IDX_PNEXT];
    }

    ptr_t& vnext() {
        return mem[IDX_VNEXT];
    }

public:

    T& data(){
        return *(static_cast<T*>(&mem[IDX_DATA_BEGIN]));
    }

    // TODO
    template<class E>
    void next(pim_list_node<E>* __next){
    
    }

    //TODO : delete
    void* allocate(int size) {
        return allocator.allocate(size);
    }

    // TODO
    pim_node* next(){
        return nullptr;
    }    

    // TODO
    template<class E>
    pim_list_node<E>* next_as(){
        return nullptr;
    }

    pim_node* next_as(){
        return next();
    }    

    void set_allocator(PimAllocator* allocator) {
        pim_list_node::allocator = allocator;
    }
};

template<typename T, class allocator_type>
PimAllocator& pim_list_node<T, allocator_type>::allocator = allocator_type::instance;