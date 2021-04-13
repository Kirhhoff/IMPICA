#include <sys/mman.h>
#include <gtest/gtest.h>
#include "pim_list_node.hpp"
#include "mock_syscall.h"

constexpr ptr_t va2pa_offset = 0x100000000000;
constexpr ptr_t BUFF_SIZE = PAGE_SIZE;

class fake_page_manager : public page_manager<fake_page_manager> {
public:
    static fake_page_manager instance;

    ptr_t query_and_lock(ptr_t vaddr){
        return vaddr + va2pa_offset;
    }

    void release(ptr_t vaddr) { }
};

template<typename T>
class mnode : public pim_list_node<T, mock_syscall, fake_page_manager> {
public:
    using Base = pim_list_node<T, mock_syscall, fake_page_manager>;

    constexpr static psize_t T_size = Base::T_size;
    
    template<typename... Args>
    mnode(Args... arg)
        :Base(arg...) 
        { }

    ptr_t pnext() { return Base::pnext(); }

    ptr_t pthis(ptr_t idx) { return Base::pthis(idx); }

    ptr_t vpn(ptr_t idx) {
        return reinterpret_cast<ptr_t>(&Base::mem[idx]) >> PAGE_SHIFT;
    }

    bool nocross() {
        return vpn(0) == vpn(Base::n_chunk - 1);
    }

    bool cross_data() {
        return vpn(Base::IDX_DATA_BEGIN - 1) != vpn(Base::n_chunk -1);
    }

    bool cross_header() {
        return vpn(0) != vpn(Base::IDX_DATA_BEGIN - 1);
    }

};

struct RepeatData {

    ptr_t r[8];
    
    RepeatData(unsigned char c) {
        r[0] = c;
        for (int i = 1; i < 8; i++) {
            r[i] = (r[i - 1] << 8) | c;
        }
    }
};

fake_page_manager fake_page_manager::instance;
ptr_t mstart;

void* pim_malloc(psize_t size, mock_tag tag) {
    static int state = 0;
    ptr_t addr = 0;

    switch (state)
    {
    case 0: addr = mstart + PAGE_SIZE * 0 / 2; // no cross, page 1
        break;
    case 1: addr = mstart + PAGE_SIZE * 2 / 2; // no cross, page 2
        break;
    case 2: addr = mstart + PAGE_SIZE / 2; // no cross, page 1
        break;
    case 3: addr = mstart + PAGE_SIZE * 3 / 2; // no cross, page 2
        break;
    case 4: addr = mstart + PAGE_SIZE - 6 * PTR_SIZE; // cross data, page 1/2
        break;
    case 5: addr = mstart + PAGE_SIZE * 3 / 2; // no cross, page 2
        break;
    case 6: addr = mstart + PAGE_SIZE * 2 - PTR_SIZE; // cross header, page 2/3
        break;
    case 7: addr = mstart + PAGE_SIZE * 5 / 2; // no cross, page 3
        break;
    default:
        break;
    }
    state++;
    return reinterpret_cast<void*>(addr);
}

void pim_free(void* ptr, mock_tag tag) { }

ptr_t pa2va(ptr_t paddr) {
    return paddr - va2pa_offset;
}

ptr_t min(ptr_t v1, ptr_t v2) {
    return v1 < v2 ? v1 : v2;
}

ptr_t mock_pim_device_find(ptr_t pthis, ptr_t ptarget, ptr_t vthis, psize_t total) {

    ptr_t buff[PAGE_SIZE], cbuff[PAGE_SIZE];
    bool header_read_finished;
    ptr_t header_read_pos, header_cnt;
    ptr_t pread, readlen, cmp_pos, end_pos;
    psize_t size, ext;

    total += 3 * PTR_SIZE;

    // readin buff
    ext = 0;
    end_pos = 0;
    header_cnt = 0;
    header_read_finished = false;
    header_read_pos = 0;
    pread = pa2va(ptarget & ~PIM_CROSS_PAGE);
    size = total;
    
    do {
        // read in
        readlen = min(size, PAGE_SIZE - (pread & PAGE_MASK));

        // printf("target readlen = %ld\n", readlen);
        // printf("target read addr = %p\n", reinterpret_cast<void*>(pread));

        memcpy(&buff[end_pos], (void*)pread, readlen);

        // for (int i = 0; i < 12; i++) {
        //     printf("buff[%d] = 0x%lx\n", i, buff[i]);
        // }

        end_pos += readlen / PTR_SIZE;
        size -= readlen;

        // printf("totoal %ld\n", total);

        // deal with header
        if (!header_read_finished) {
            while (header_cnt < end_pos && (buff[header_cnt] & PIM_META_END) == 0) {
                header_cnt++;
                ext += PTR_SIZE;
            }

            // printf("header cnt = [%ld] end pos = [%ld] size left = [%ld]\n", header_cnt, end_pos, size);
            
            if (header_cnt < end_pos) {
                header_cnt++;
                ext += PTR_SIZE;   
                header_read_finished = true;
                if (size == 0) {
                    
                    size = ext;
                    total += ext;
                    pread += readlen;
                    continue; 
                }
            }
            size += ext;
            total += ext;
            ext = 0; 
        }

        pread = pa2va(buff[header_read_pos++] & ~(PIM_CROSS_PAGE | PIM_META_END));

        // printf("pread %p\n", reinterpret_cast<void*>(pread));

    } while (size > 0);

    // printf("get out!\n");
    
    while (pthis != 0){
        end_pos = 0;
        header_cnt = 0;
        header_read_finished = false;
        header_read_pos = 0;
        pread = pa2va(pthis & ~PIM_CROSS_PAGE);
        size = total;

        do {
            // read in
            readlen = min(size, PAGE_SIZE - (pread & PAGE_MASK));
            memcpy(&cbuff[end_pos], (void*)pread, readlen);
            end_pos += readlen / PTR_SIZE;
            size -= readlen;

            // deal with header
            if (!header_read_finished) {
                while (header_cnt < end_pos && (cbuff[header_cnt] & PIM_META_END) == 0) {
                    header_cnt++;
                }
                if (header_cnt < end_pos) {
                    header_cnt++;
                    header_read_finished = true;
                    cmp_pos = header_cnt + 3;
                    if (cmp_pos >= end_pos) {
                        goto next;
                    }
                } else {
                    goto next;
                }
            }

            if (memcmp(&cbuff[cmp_pos], &buff[cmp_pos], (end_pos - cmp_pos) * PTR_SIZE) != 0) {
                break;
            }
            cmp_pos = end_pos;

next:
            pread = pa2va(cbuff[header_read_pos++] & ~(PIM_CROSS_PAGE | PIM_META_END));
        } while (size > 0);

        if (cmp_pos == end_pos) {
            return vthis;
        } else {
            pthis = cbuff[header_cnt];
            vthis = cbuff[header_cnt + 1];
        }
    }

    return 0;
}

template<typename T>
mnode<T>* mock_pim_find(mnode<T>* begin, T& val) {
    mnode<T> target(val);
    return reinterpret_cast<mnode<T>*>(mock_pim_device_find(
        begin->pthis(0),
        target.pthis(0),
        reinterpret_cast<ptr_t>(begin), sizeof(T)));
}

TEST(PimListNodeTests, LinkAndFind) {
    mstart = reinterpret_cast<ptr_t>(mmap(0, PAGE_SIZE * 3,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        0, 0));
    
    RepeatData dumpy(0xEE);
    
    using node = mnode<RepeatData>;

    auto* n1 = new node(0xAA);
    auto* n2 = new node(0xBB);
    auto* n3 = new node(0xCC);
    auto* n4 = new node(0xDD);
    
    // printf("n1 %p\n", n1);
    // printf("n2 %p\n", n2);
    // printf("n3 %p\n", n3);
    // printf("n4 %p\n", n4);
    
    ASSERT_TRUE(n1->nocross());
    ASSERT_TRUE(n2->nocross());
    ASSERT_TRUE(n3->nocross());
    ASSERT_TRUE(n4->nocross());

    n1->next(n2);
    n2->next(n3);
    n3->next(n4);

    ASSERT_EQ(n1->next(), n2);
    ASSERT_EQ(n2->next(), n3);
    ASSERT_EQ(n3->next(), n4);
    ASSERT_EQ(n4->next(), nullptr);

    ASSERT_EQ(mock_pim_find(n1, n1->data()), n1);
    ASSERT_EQ(mock_pim_find(n1, n2->data()), n2);
    ASSERT_EQ(mock_pim_find(n1, n3->data()), n3);
    ASSERT_EQ(mock_pim_find(n1, n4->data()), n4);
    ASSERT_EQ(mock_pim_find(n1, dumpy), nullptr);

    delete n1;
    delete n2;
    delete n3;
    delete n4;

    n1 = new node(0xAA);
    n2 = new node(0xBB);
    n3 = new node(0xCC);
    n4 = new node(0xDD);

    // printf("n1 %p\n", n1);
    // printf("n2 %p\n", n2);
    // printf("n3 %p\n", n3);
    // printf("n4 %p\n", n4);
    
    ASSERT_TRUE(n1->cross_data());
    ASSERT_TRUE(n2->nocross());
    ASSERT_TRUE(n3->cross_header());
    ASSERT_TRUE(n4->nocross());

    n1->next(n2);
    n2->next(n3);
    n3->next(n4);

    ASSERT_EQ(n1->next(), n2);
    ASSERT_EQ(n2->next(), n3);
    ASSERT_EQ(n3->next(), n4);
    ASSERT_EQ(n4->next(), nullptr);

    ASSERT_EQ(mock_pim_find(n1, n1->data()), n1);
    ASSERT_EQ(mock_pim_find(n1, n2->data()), n2);
    ASSERT_EQ(mock_pim_find(n1, n3->data()), n3);
    ASSERT_EQ(mock_pim_find(n1, n4->data()), n4);
    ASSERT_EQ(mock_pim_find(n1, dumpy), nullptr);

    n2->clear_next();
    
    ASSERT_EQ(n1->next(), n2);
    ASSERT_EQ(n2->next(), nullptr);

    ASSERT_EQ(mock_pim_find(n1, n1->data()), n1);
    ASSERT_EQ(mock_pim_find(n1, n2->data()), n2);
    ASSERT_EQ(mock_pim_find(n1, n3->data()), nullptr);
    ASSERT_EQ(mock_pim_find(n1, n4->data()), nullptr);
    ASSERT_EQ(mock_pim_find(n1, dumpy), nullptr);
    ASSERT_EQ(mock_pim_find(n3, n3->data()), n3);
    ASSERT_EQ(mock_pim_find(n3, n4->data()), n4);
    ASSERT_EQ(mock_pim_find(n3, dumpy), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}