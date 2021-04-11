#ifndef PIM_DEFS_H
#define PIM_DEFS_H

#include <cstdint>
typedef uintptr_t ptr_t;
typedef std::size_t psize_t;

typedef ptr_t chunk_t;

constexpr psize_t PAGE_SHIFT = 12;
constexpr psize_t PAGE_SIZE = 1 << PAGE_SHIFT;

constexpr psize_t PTR_SIZE = sizeof(ptr_t);
constexpr psize_t N_PTR_PER_PAGE = PAGE_SIZE / PTR_SIZE;

inline constexpr psize_t round_up(const psize_t size, const psize_t round_unit) {
    return (size + round_unit - 1) / round_unit * round_unit;
};

#endif