#include "pim_list_node.hpp"
#include "mock_allocator.h"
#include "mock_page_manager.h"
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::_;

template<psize_t n>
struct mock_data{
    ptr_t data[n];
};

template<psize_t n>
struct mock_node : public pim_list_node<mock_data<n>, mock_syscall> {
    static constexpr psize_t n_est_meta = pim_list_node<mock_data<n>, mock_syscall>::n_est_meta_ptr;    
    static constexpr psize_t n_meta = pim_list_node<mock_data<n>, mock_syscall>::n_pim_meta_ptr;    
    static constexpr psize_t n_chunk = pim_list_node<mock_data<n>, mock_syscall>::n_chunk;    
};

TEST(BasicTests, PimSizeCalculate) {

    constexpr psize_t size1 = N_PTR_PER_PAGE - 3;
    constexpr psize_t size2 = N_PTR_PER_PAGE - 2;
    constexpr psize_t size3 = N_PTR_PER_PAGE;
    constexpr psize_t size4 = 2 * N_PTR_PER_PAGE - 4;
    constexpr psize_t size5 = 2 * N_PTR_PER_PAGE - 3;
    constexpr psize_t size6 = 2 * N_PTR_PER_PAGE;

    typedef mock_node<size1> mock1;
    typedef mock_node<size2> mock2;
    typedef mock_node<size3> mock3;
    typedef mock_node<size4> mock4;
    typedef mock_node<size5> mock5;
    typedef mock_node<size6> mock6;

    ASSERT_EQ(mock1::n_est_meta, 2);
    ASSERT_EQ(mock2::n_est_meta, 2);
    ASSERT_EQ(mock3::n_est_meta, 3);
    ASSERT_EQ(mock4::n_est_meta, 3);
    ASSERT_EQ(mock5::n_est_meta, 3);
    ASSERT_EQ(mock6::n_est_meta, 4);

    ASSERT_EQ(mock1::n_meta, 2);
    ASSERT_EQ(mock2::n_meta, 3);
    ASSERT_EQ(mock3::n_meta, 3);
    ASSERT_EQ(mock4::n_meta, 3);
    ASSERT_EQ(mock5::n_meta, 4);
    ASSERT_EQ(mock6::n_meta, 4);

    ASSERT_EQ(mock1::n_chunk, size1 + 4);
    ASSERT_EQ(mock2::n_chunk, size2 + 5);
    ASSERT_EQ(mock3::n_chunk, size3 + 5);
    ASSERT_EQ(mock4::n_chunk, size4 + 5);
    ASSERT_EQ(mock5::n_chunk, size5 + 6);
    ASSERT_EQ(mock6::n_chunk, size6 + 6);
}

struct operator_new_tester : public pim_list_node<mock_data<8>> {
    
    static void* operator new(psize_t count) {
        return pim_list_node<mock_data<8>>::operator new(count);
    }
    
    ptr_t vthis() {
        return pthis(0); 
    }
};


TEST(BasicTests, OperatorNewOverload) {
    operator_new_tester* tester = new operator_new_tester;
    ASSERT_EQ(tester->vthis(), (ptr_t)tester);
}

mock_page_manager mock_page_manager::instance;
using testing::_;
using testing::Return;

TEST(BasicTests, PageManager) {
    EXPECT_CALL(mock_page_manager::instance, query_and_lock(_))
        .WillRepeatedly(Return(0xABCDABCD));
    pim_list_node<int, system_syscall, mock_page_manager> t;
    mock_page_manager::instance.~mock_page_manager();    
}

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

ptr_t va2pa(ptr_t, mock_tag tag) {
    pid_t pid = getpid();
    printf("pid: %d\n", pid);
    char filename[64];
    snprintf(filename, sizeof(filename), "/proc/%d/pagemap", pid);
    int fd;
    fd = open(filename, O_RDONLY);
    printf("fd: %d\n", fd);
    ptr_t addr = ((ptr_t)(&pid) & ~(PAGE_SIZE - 1));
    ptr_t off = addr / PAGE_SIZE * 8;
    uint64_t entry;
    int res;
    res = mlock((void*)addr, PAGE_SIZE);
    printf("lock res: %d\n", res);
    res = pread(fd, &entry, sizeof(entry), off);
    printf("&pid = 0x%lx, addr = 0x%lx off = 0x%lx\n", &pid, addr, off);
    printf("read res: %d\n", res);
    printf("entry: 0x%lx\n", entry);
    printf("pfn: 0x%lx\n", (entry & ((1 << 55) - 1)));
    printf("P: 0x%lx\n", entry >> 63);

    // pread()
}

int main(int argc, char** argv) {
    va2pa(0,mock_tag());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}