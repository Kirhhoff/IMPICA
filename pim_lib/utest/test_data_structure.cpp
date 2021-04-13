#include "mock_syscall.h"
#include "pim_list_node.hpp"
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unordered_set>

template<psize_t nptr>
struct mock_data{
    ptr_t data[nptr];
};

template<psize_t n, class page_manager_impl = default_page_manager<mock_syscall>>
struct mock_node : public pim_list_node<mock_data<n>, mock_syscall, page_manager_impl> {
    using Base = pim_list_node<mock_data<n>, mock_syscall, page_manager_impl>;
    static constexpr psize_t n_est_meta = Base::n_est_meta_ptr;    
    static constexpr psize_t n_meta = Base::n_pim_meta_ptr;    
    static constexpr psize_t n_chunk = Base::n_chunk;

    ptr_t pthis(psize_t idx) { return Base::pthis(idx); }
    ptr_t pnext() { return Base::pnext(); }
    ptr_t vnext() { return Base::vnext(); }
};

class fake_page_manager : public page_manager<fake_page_manager>, public default_page_manager<mock_syscall>{
public:
    static fake_page_manager instance;

    using Base = default_page_manager<mock_syscall>;
    
    ptr_t query_and_lock(ptr_t vaddr) {
        return Base::query_and_lock(vaddr);
    }

    void release(ptr_t vaddr) { 
        Base::release(vaddr);
    }

    auto& get_map(){
        return va2pa_map;
    }
};

constexpr psize_t extra = 2;
constexpr ptr_t va2pa_offset = 0x10000;

template<psize_t nptr>
using node = mock_node<nptr, fake_page_manager>;

using mock_nocross = node<1>; 
using mock_cross_1 = node<2 * extra>;
using mock_cross_2 = node<N_PTR_PER_PAGE + 2 * extra>; 

using std::unordered_set;
using pc_pair = std::pair<ptr_t, psize_t>;

fake_page_manager fake_page_manager::instance;
unordered_set<ptr_t> locked_page;
unordered_map<ptr_t, psize_t> va2pa_call_cnt;
ptr_t malloc_start_addr;

bool operator==(const pc_pair& p1, const pc_pair& p2) {
    return p1.first == p2.first && p1.second == p2.second; 
}

void* pim_malloc(psize_t size, mock_tag tag) {
    static int state = 0;
    ptr_t addr;
    switch (state)
    {
    case 0: addr = malloc_start_addr + PAGE_SIZE / 2; 
        break;
    case 1: addr = malloc_start_addr + PAGE_SIZE - extra * PTR_SIZE;
        break;
    case 2: addr = malloc_start_addr + 2 * PAGE_SIZE - extra * PTR_SIZE;
        break;
    default: addr = 0;
        break;
    }
    state++;
    return reinterpret_cast<void*>(addr); 
}

void pim_free(void* ptr, mock_tag tag) { }

int pim_mlock(void* addr, psize_t len, mock_tag) {
    ptr_t vstart = reinterpret_cast<ptr_t>(addr) & ~PAGE_MASK;
    ptr_t vend = (reinterpret_cast<ptr_t>(addr) + len - 1) & ~PAGE_MASK;
    for (auto vpage = vstart; vpage <= vend; vpage += PAGE_SIZE) {
        locked_page.insert(vpage);
    } 
    return 0;
}

int pim_munlock(void* addr, psize_t len, mock_tag) {
    ptr_t vstart = reinterpret_cast<ptr_t>(addr) & ~PAGE_MASK;
    ptr_t vend = (reinterpret_cast<ptr_t>(addr) + len - 1) & ~PAGE_MASK;
    for (auto vpage = vstart; vpage <= vend; vpage += PAGE_SIZE) {
        locked_page.erase(vpage);
    } 
    return 0;
}

ptr_t va2pa(ptr_t addr, mock_tag tag) {
    va2pa_call_cnt[addr]++;
    return addr + va2pa_offset;
}

TEST(PimListNodeTests, PimSizeCalculate) {

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

TEST(PageManagerTests, LockAndVa2paCallCnt) {
    auto& pm = default_page_manager<mock_syscall>::instance;

    ptr_t va1 = 0x1000;
    ptr_t va2 = 0x2000;
    ptr_t va3 = 0x1008;
    ptr_t pa;
    
    locked_page.clear();
    va2pa_call_cnt.clear();

    pa = pm.query_and_lock(va1);
    ASSERT_EQ(locked_page.size(), 1);
    ASSERT_TRUE(locked_page.find(va1 & ~PAGE_MASK) != locked_page.end());
    ASSERT_EQ(va2pa_call_cnt.size(), 1);
    ASSERT_EQ(va2pa_call_cnt[va1 & ~PAGE_MASK], 1);
    ASSERT_EQ(pa, va1 + va2pa_offset);


    pa = pm.query_and_lock(va2);
    ASSERT_EQ(locked_page.size(), 2);
    ASSERT_TRUE(locked_page.find(va1 & ~PAGE_MASK) != locked_page.end());
    ASSERT_TRUE(locked_page.find(va2 & ~PAGE_MASK) != locked_page.end());
    ASSERT_EQ(va2pa_call_cnt.size(), 2);
    ASSERT_EQ(va2pa_call_cnt[va1 & ~PAGE_MASK], 1);
    ASSERT_EQ(va2pa_call_cnt[va2 & ~PAGE_MASK], 1);
    ASSERT_EQ(pa, va2 + va2pa_offset);

    pa = pm.query_and_lock(va3);
    ASSERT_EQ(locked_page.size(), 2);
    ASSERT_TRUE(locked_page.find(va2 & ~PAGE_MASK) != locked_page.end());
    ASSERT_TRUE(locked_page.find(va3 & ~PAGE_MASK) != locked_page.end());
    ASSERT_EQ(va2pa_call_cnt.size(), 2);
    ASSERT_EQ(va2pa_call_cnt[va2 & ~PAGE_MASK], 1);
    ASSERT_EQ(va2pa_call_cnt[va3 & ~PAGE_MASK], 1);
    ASSERT_EQ(pa, va3 + va2pa_offset);

    pm.release(va1);
    ASSERT_EQ(locked_page.size(), 2);
    ASSERT_TRUE(locked_page.find(va2 & ~PAGE_MASK) != locked_page.end());
    ASSERT_TRUE(locked_page.find(va3 & ~PAGE_MASK) != locked_page.end());

    pm.release(va3);
    ASSERT_EQ(locked_page.size(), 1);
    ASSERT_TRUE(locked_page.find(va2 & ~PAGE_MASK) != locked_page.end());

    pm.release(va2);
    ASSERT_TRUE(locked_page.empty());
}


TEST(PimListNodeTests, ConstructorDestructor) {
    malloc_start_addr = reinterpret_cast<ptr_t>(mmap(0, 4 * PAGE_SIZE, 
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        0, 0));
    auto& map = fake_page_manager::instance.get_map();

    ASSERT_TRUE(map.empty());

    auto* m1 = new mock_nocross;
    ASSERT_EQ(m1->pnext(), 0);
    ASSERT_EQ(m1->vnext(), 0);
    ASSERT_EQ(m1->pthis(0), va2pa_offset + malloc_start_addr + PAGE_SIZE / 2);
    ASSERT_EQ(m1->pthis(1), 0 | PIM_META_END);
    ASSERT_EQ(map.size(), 1);
    ASSERT_TRUE(map.find(malloc_start_addr) != map.end());
    ASSERT_EQ(map[malloc_start_addr], pc_pair(va2pa_offset + malloc_start_addr, 1));

    auto* m2 = new mock_cross_1;
    ASSERT_EQ(m2->pnext(), 0);
    ASSERT_EQ(m2->vnext(), 0);
    ASSERT_EQ(m2->pthis(0), (va2pa_offset + malloc_start_addr + PAGE_SIZE - extra * PTR_SIZE) | PIM_CROSS_PAGE);
    ASSERT_EQ(m2->pthis(1), (va2pa_offset + malloc_start_addr + PAGE_SIZE) | PIM_META_END);
    ASSERT_EQ(map.size(), 2);
    ASSERT_TRUE(map.find(malloc_start_addr) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + PAGE_SIZE) != map.end());
    ASSERT_EQ(map[malloc_start_addr], pc_pair(va2pa_offset + malloc_start_addr, 2));
    ASSERT_EQ(map[malloc_start_addr + PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + PAGE_SIZE, 1));
    

    auto* m3 = new mock_cross_2;
    ASSERT_EQ(m3->pnext(), 0);
    ASSERT_EQ(m3->vnext(), 0);
    ASSERT_EQ(m3->pthis(0), (va2pa_offset + malloc_start_addr + 2 * PAGE_SIZE - extra * PTR_SIZE) | PIM_CROSS_PAGE);
    ASSERT_EQ(m3->pthis(1), (va2pa_offset + malloc_start_addr + 2 * PAGE_SIZE) | PIM_CROSS_PAGE);
    ASSERT_EQ(m3->pthis(2), (va2pa_offset + malloc_start_addr + 3 * PAGE_SIZE) | PIM_META_END);
    ASSERT_EQ(map.size(), 4);
    ASSERT_TRUE(map.find(malloc_start_addr) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + PAGE_SIZE) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + 2 * PAGE_SIZE) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + 3 * PAGE_SIZE) != map.end());
    ASSERT_EQ(map[malloc_start_addr], pc_pair(va2pa_offset + malloc_start_addr, 2));
    ASSERT_EQ(map[malloc_start_addr + PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + PAGE_SIZE, 2));
    ASSERT_EQ(map[malloc_start_addr + 2 * PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + 2 * PAGE_SIZE, 1));
    ASSERT_EQ(map[malloc_start_addr + 3 * PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + 3 * PAGE_SIZE, 1));

    delete m1;
    ASSERT_EQ(map.size(), 4);
    ASSERT_TRUE(map.find(malloc_start_addr) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + PAGE_SIZE) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + 2 * PAGE_SIZE) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + 3 * PAGE_SIZE) != map.end());
    ASSERT_EQ(map[malloc_start_addr], pc_pair(va2pa_offset + malloc_start_addr, 1));
    ASSERT_EQ(map[malloc_start_addr + PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + PAGE_SIZE, 2));
    ASSERT_EQ(map[malloc_start_addr + 2 * PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + 2 * PAGE_SIZE, 1));
    ASSERT_EQ(map[malloc_start_addr + 3 * PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + 3 * PAGE_SIZE, 1));

    delete m3;
    ASSERT_EQ(map.size(), 2);
    ASSERT_TRUE(map.find(malloc_start_addr) != map.end());
    ASSERT_TRUE(map.find(malloc_start_addr + PAGE_SIZE) != map.end());
    ASSERT_EQ(map[malloc_start_addr], pc_pair(va2pa_offset + malloc_start_addr, 1));
    ASSERT_EQ(map[malloc_start_addr + PAGE_SIZE], pc_pair(va2pa_offset + malloc_start_addr + PAGE_SIZE, 1));

    delete m2;
    ASSERT_TRUE(map.empty());
}

struct CtorMock{
    CtorMock(ptr_t f1, ptr_t f2, ptr_t f3, ptr_t f4, ptr_t f5)
        : field1(f1), field2(f2), field3(f3), field4(f4), field5(f5)
        { }
    ptr_t field1;
    ptr_t field2;
    ptr_t field3;
    ptr_t field4;
    ptr_t field5;
    bool operator==(const CtorMock& obj) const{
        return field1 == obj.field1 &&
            field2 == obj.field2 &&
            field3 == obj.field3 &&
            field4 == obj.field4 &&
            field5 == obj.field5;
    }
};


TEST(PimListNodeTests, ConstructorForward) {
    pim_list_node<CtorMock> stack_node(1, 2, 3, 4, 5);
    ASSERT_EQ(stack_node.data(), CtorMock(1, 2, 3, 4, 5));

    auto* heap_node = new pim_list_node<CtorMock>(1, 2, 3, 4, 5); 
    ASSERT_EQ(heap_node->data(), CtorMock(1, 2, 3, 4, 5));
    delete heap_node;
}

int main(int argc, char** argv) {
    // auto* t1 = new pim_list_node<mock_data<1>>;
    // auto* t2 = new pim_list_node<mock_data<512>>;
    // auto* t3 = new pim_list_node<mock_data<1024>>;
    // delete t1;
    // delete t3;
    // delete t2;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}