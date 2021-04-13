#include <gtest/gtest.h>
#include "pim_list_node.hpp"
#include "mock_syscall.h"

constexpr ptr_t va2pa_offset = 0x100000;

class fake_page_manager : public page_manager<fake_page_manager> {
public:
    ptr_t query_and_lock(ptr_t vaddr){
        return vaddr + va2pa_offset;
    }

    void release(ptr_t vaddr) {

    }
};

template<typename T>
class mnode : public pim_list_node<T, mock_syscall, fake_page_manager> {
public:
    using Base = pim_list_node<T, mock_syscall, fake_page_manager>;
    constexpr static psize_t T_size = Base::T_size;
    ptr_t pnext() { return Base::pnext(); }
};

void* pim_malloc(psize_t size, mock_tag tag) {
    return 0;
}

void pim_free(void* ptr, mock_tag tag) {

}

ptr_t pa2va(ptr_t paddr) {
    return paddr - va2pa_offset;
}

ptr_t min(ptr_t v1, ptr_t v2) {
    return v1 < v2 ? v1 : v2;
}

// template<typename T>
// mnode<T>* mock_pfind(mnode<T>* nodep, T& val) {
//     mnode<T>* target = new mnode<T>(val);
//     bool cross = reinterpret_cast<ptr_t>(nodep) & PIM_CROSS_PAGE;
//     ptr_t size = mnode<T>::T_size;
//     while (nodep) {
//         ptr_t pstart = nodep & ~PIM_CROSS_PAGE; 
//         ptr_t readlen;
//         do {
//             readlen = min(size, PAGE_SIZE - (pstart & PAGE_MASK));
//             if(memcmp() != 0) {
//                 break;
//             }
//             size -= readlen;
//             ptr_t addr = 
//         } while (cross);
        
//         cross = reinterpret_cast<ptr_t>(nodep) & PIM_CROSS_PAGE;
//         while (cross) {
            
//             /* code */
//         }

//         nodep = reinterpret_cast<mnode<T>*>(nodep->pnext());
//     }
//     return nullptr;
// }
#define BUFF_SIZE PAGE_SIZE

ptr_t mock_pim_device_find(ptr_t pthis, ptr_t ptarget, ptr_t vthis, psize_t total) {
    assert(total < BUFF_SIZE);
    ptr_t buff[PAGE_SIZE], cbuff[PAGE_SIZE];
    bool header_read_finished;
    ptr_t header_read_pos, header_cnt;
    ptr_t pread, readlen, cmp_pos, end_pos;
    psize_t size;

    // readin buff
    end_pos = 0;
    header_cnt = 0;
    header_read_finished = false;
    header_read_pos = 0;
    pread = ptarget & ~PIM_CROSS_PAGE;
    size = total;
    
    do {
        // read in
        readlen = min(size, PAGE_SIZE - (pread & PAGE_MASK));
        memcpy(&buff[end_pos], (void*)pread, readlen);
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
            } 
        }
        pread = pa2va(cbuff[header_read_pos++] & ~(PIM_CROSS_PAGE | PIM_META_END));
    } while (size > 0);

    while (pthis != 0){
        end_pos = 0;
        header_cnt = 0;
        header_read_finished = false;
        header_read_pos = 0;
        pread = pthis & ~PIM_CROSS_PAGE;
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
                        // header_read_pos++;
                        // continue;
                    }
                } else {
                    goto next;
                    // header_read_pos++;
                    // continue;
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

TEST(PimListNodeTests, Linking) {

}

struct RepeatData {
    ptr_t r[8];
    RepeatData(unsigned char c) {
        r[0] = c;
        for (int i = 1; i < 8; i++) {
            r[i] = (r[i - 1] << 8) | c;
        }
    }
};
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}