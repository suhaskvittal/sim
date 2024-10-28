/*
 *  author: Suhas Vittal
 *  date:   15 October 2024
 * */

#include "ds3/interface.h"
#include "cache/controller/llc2.h"
#include "utils/bitcount.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DS3Interface::DS3Interface(std::string ds3conf) {
    mem_ = ds3::GetMemorySystem( ds3conf, ".",
            [this] (uint64_t byteaddr)
            {
                uint64_t lineaddr = byteaddr >> Log2<LINESIZE>::value;
                GL_llc_controller_->mark_as_finished(lineaddr);
            },
            [this] (uint64_t byteaddr)
            {}
        );
}

DS3Interface::~DS3Interface() {
    delete mem_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DS3Interface::make_request(uint64_t lineaddr, bool is_read) {
    uint64_t byteaddr = lineaddr << Log2<LINESIZE>::value;
    if (!mem_->WillAcceptTransaction(byteaddr, !is_read)) {
        ++s_queue_full_;
        return false;
    }
    mem_->AddTransaction(byteaddr, !is_read);
    return true;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DS3Interface::print_stats(void) {
    PRINT_STAT(std::cout, "MEM_FAILED_REQUESTS", s_queue_full_);
    std::cout << "\n";
    mem_->PrintStats(true); 
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
