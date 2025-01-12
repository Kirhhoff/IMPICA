#ifndef PIM_DEFS_H
#define PIM_DEFS_H

#include <cstdint>

typedef uintptr_t ptr_t;
typedef std::size_t psize_t;

typedef ptr_t chunk_t;

constexpr psize_t PAGE_SHIFT = 12;
constexpr psize_t PAGE_SIZE = 1 << PAGE_SHIFT;
constexpr psize_t PAGE_MASK = PAGE_SIZE - 1;

constexpr ptr_t PIM_CROSS_PAGE_SHIFT = 0;
constexpr ptr_t PIM_CROSS_PAGE = 1 << PIM_CROSS_PAGE_SHIFT;
constexpr ptr_t PIM_META_END_SHIFT = 1;
constexpr ptr_t PIM_META_END = 1 << PIM_META_END_SHIFT;

constexpr psize_t PTR_SIZE = sizeof(ptr_t);
constexpr psize_t N_PTR_PER_PAGE = PAGE_SIZE / PTR_SIZE;

constexpr void* PIM_MAP_FAILED = reinterpret_cast<void*>(-1);
constexpr int Done           = 0x01C;
constexpr int Pthis			= 0x038;
constexpr int Ptarget		= 0x040;
constexpr int Vthis			= 0x048;
constexpr int Tsize			= 0x050;

inline constexpr psize_t round_up(const psize_t size, const psize_t round_unit) {
    return (size + round_unit - 1) / round_unit * round_unit;
};

#endif