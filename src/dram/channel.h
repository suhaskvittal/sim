/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_CHANNEL_h
#define DRAM_CHANNEL_h

#include "dram/bank.h"
#include "dram/config.h"

/*
 * Command queue structures.
 * */
enum class CmdStatus { INVALID, WAITING, FIRED, DONE };
enum class CmdType { READ, WRITE, REFRESH };

struct CmdQueueEntry {
    CmdType cmd_type_;
    /*
     * Addressing: not all values are necessarily used.
     * */
    size_t row_;
    size_t bank_;
    size_t bgrp_;
    size_t rank_;
    /*
     * Tracks when the CMD left and exited the command queue,
     * and when it finished.
     * */
    uint64_t t_entered_queue_;
    uint64_t t_exited_queue_;
    uint64_t t_finished_;
};
/*
 * Rank data structures. These have no functions, and only contain
 * information for bookkeeping.
 * */
struct RankData {
    RankData(const DRAMConfig& conf)
        :banks_( conf.num_bgrps, std::vector<DRAMBank>(conf.num_banks_per_bgrp, DRAMBank(conf)) )
    {}
    /*
     * Necessary to track for timing.
     * */
    size_t last_bgrp_ =0;
    size_t last_bank_ =0;
    uint64_t last_bank_cycle_ =0;
    bool last_bank_was_written_to_ =false;
    /*
     * Banks organized by bankgroup.
     * */ 
    std::vector< std::vector<DRAMBank> > banks_;
    /*
     * Cycles for the last four activations. If `faw_cycles.size() == 4`, then no more ACTs can be
     * done. Once `cycle_` is greater than a given entry in `faw_cycles`, the corresponding entry
     * is removed.
     * */
    std::vector<uint64_t> faw_cycles_;
    /*
     * A bitvector for tracking banks that have done a REF in the last tREFI. 
     * This is active-low to use with `ffsll`.
     * */
    uint64_t banks_done_with_curr_trefi_ =~0;
    uint64_t next_trefi_cycle_ =0;
};

class DRAMChannel {
public:
    constexpr static size_t CMD_QUEUE_SIZE = 32;

    uint64_t cycle_ =0;
    std::vector<RankData> ranks_;

    const DRAMConfig& conf_;
    const DRAMAddressMapping& am_;
private:
    CmdQueueEntry cmd_queue_[CMD_QUEUE_SIZE];
    size_t cmd_queue_ptr_ =0;
    size_t cmd_queue_size_ =0;
public:
    DRAMChannel(const DRAMConfig&);

    void tick(void);

    bool push_back_cmd(CmdQueueEntry&&);
private:
    void check_faw(void);
    void manage_refresh(void);
    void execute_next_cmd(void);
};

#endif  // DRAM_CHANNEL_h
