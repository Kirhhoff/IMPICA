#include "pim_allocator.h"



class MallocAllocatorWrapper : public PimAllocator {

public:
    static MallocAllocatorWrapper instance;

    virtual void* allocate(int size) override {
        return (void*)0xBEEFBEEF;
    } 

};