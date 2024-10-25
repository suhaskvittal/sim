/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_BANK_h
#define DRAM_BANK_h

#include "dram/config.h"

#include <vector>

/*
 * Declarations of classes/structs that are defined in `channel.h`.
 * */
struct CmdQueueEntry;

struct DRAMSubarray {
    int64_t open_row_ =-1;
};
/*
 * DRAMBank structure models basic latencies for operations.
 * Note that delays are incomplete by themselves as bankgroup
 * parameters are not used (i.e. tCCD_L and tCCD_S): these must
 * be modeled by `DRAMChannel`.
 * */
class DRAMBank {
public:
    constexpr static size_t SUBARRAY_SIZE = 512;

    uint64_t busy_until_cycle_=0;

    uint64_t s_row_buf_misses_=0;
    uint64_t s_bank_accesses_=0;

    const DRAMConfig& conf_;
private:
    /*
     * Refresh tracking: every tREFI, a group of 8K rows are refreshed. When
     * this occurs, the bank is blocked for tRFC.
     * */
    size_t num_refresh_groups_;
    size_t curr_refresh_group_=0;

    std::vector<DRAMSubarray> subarrays_;
public:
    DRAMBank(const DRAMConfig&);

    bool do_cmd(uint64_t cycle, CmdQueueEntry&);
    bool is_row_in_row_buffer(size_t) const;
};

#endif  // DRAM_BANK_h
