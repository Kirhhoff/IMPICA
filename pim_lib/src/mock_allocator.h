#include "pim_allocator.h"
#include <gmock/gmock.h>

class MockAllocator : public PimAllocator {
public:
    static MockAllocator instance;  
    // MockAllocator(){}
    MOCK_METHOD(void*, allocate, (int),(override));
};