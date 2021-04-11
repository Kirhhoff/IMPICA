#ifndef DEFAULT_PAGE_MANAGER_H
#define DEFAULT_PAGE_MANAGER_H

#include "page_manager.hpp"

class default_page_manager : public page_manager<default_page_manager> {
public:

    static default_page_manager instance;

    // TODO
    ptr_t query_and_lock(ptr_t vaddr) {
        return 0xAAAAAAAA;
    }

};

#endif