#include "pim_list_node.hpp"
#include "mock_allocator.h"
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::_;
// using namespace ::testing;



TEST(FUCK, FUCK) {

    pim_list_node<unsigned long, MockAllocator> p;
    MockAllocator& instance = MockAllocator::instance;
    // p.set_allocator(&instance);
    // MockAllocator instance;    
    EXPECT_CALL(instance, allocate(_))
        .WillOnce(Return((void*)0))
        .WillRepeatedly(Return((void*)0xABCDABCD));

    printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    // testing::Mock::AllowLeak(&instance);
    instance.~MockAllocator();
}

int main(int argc, char** argv) {
    // MallocAllocatorWrapper allocator;
    // pim_list_node<unsigned long> p;
    // p.set_allocator(&allocator);
    // printf("addr: 0x%lx\n", (unsigned long)p.allocate(0));
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}