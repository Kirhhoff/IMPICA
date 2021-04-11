#include "page_manager.hpp"
#include "gmock/gmock.h"


class mock_page_manager : public page_manager<mock_page_manager> {

public:
    static mock_page_manager instance;

    MOCK_METHOD(ptr_t, query_and_lock, (ptr_t));
    
    // ptr_t query_and_lock(ptr_t){
    //     return 0xBBBBBBBB;
    // }

};