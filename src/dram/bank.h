/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_BANK_h
#define DRAM_BANK_h

#include "dram/config.h"

#include <vector>

class DRAMBank {
public:
    constexpr static size_t REF_GROUP_SIZE = 8192;
    /*
     * `open_row_ == -1` means no row is in the global row buffer.
     * */
    int64_t open_row_ =-1;

    uint64_t s_row_buf_misses_ =0;
    uint64_t s_row_buf_accesses_ =0;

    const DRAMConfig& conf_;
private:
    /*
     * Refresh tracking: every tREFI, a group of 8K rows are refreshed. When
     * this occurs, the bank is blocked for tRFC.
     * */
    size_t num_refresh_groups_;
    size_t curr_refresh_group_ =0;
};

#endif  // DRAM_BANK_h
