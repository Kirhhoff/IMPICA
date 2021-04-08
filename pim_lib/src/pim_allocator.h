#ifndef PIM_ALLOCATOR_H
#define PIM_ALLOCATOR_H


class PimAllocator {

public: 
    virtual void* allocate(int size) = 0; 
};

#endif