/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#include "dram/bank.h"
#include "dram/channel.h"
#include "utils/bitcount.h"

DRAMBank::DRAMBank(DRAMConfig& conf)
    :conf_(conf),
    num_refresh_groups_(num_rows >> 13),
    subarrays_( num_rows >> BitCount<SUBARRAY_SIZE>::value )
{}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMBank::do_cmd(uint64_t cycle, CmdQueueEntry& e) {
    if (cycle < busy_until_cycle_) return false;

    if (e.cmd_type == CmdType::REFRESH) {
        // Blcok the bank for tRFC.
        busy_until_cycle_ = cycle + conf_.tRFC;
        // Update `curr_refresh_group_`.
        curr_refresh_group_ = (curr_refresh_group_==num_refresh_groups_) ? 0 : curr_refresh_group_+1;
    } else { // Regular demand access.
        DRAMSubarray& sa = subarrays_[ e.row_ >> BitCount<SUBARRAY_SIZE>::value ];
        uint64_t delay = 0;

        ++s_bank_accesses_;
        if (sa.open_row >= 0 && sa.open_row != e.row_) {
            ++s_row_buf_misses_;
            delay += conf_.tRP;
        }
        delay += conf_.tRCD + conf_.CL;
        if (conf_.page_policy == PagePolicy::OPEN) {
            sa.open_row = e.row_;
        } else {
            delay += conf_.tRP;
        }
        busy_until_cycle_ = cycle + delay;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMBank::is_row_in_row_buffer(size_t r) const {
    return subarrays_[ r >> BitCount<SUBARRAY_SIZE>::value ] == r;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
