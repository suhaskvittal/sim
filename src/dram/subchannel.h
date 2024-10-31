/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_SUBCHANNEL_h
#define DRAM_SUBCHANNEL_h

#include "defs.h"
#include "dram/rank.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct DRAMTransaction {
    uint64_t lineaddr_;
    uint64_t cycle_added_;
    uint64_t cycle_fired_;
    uint64_t cycle_finished_;

    DRAMTransaction(uint64_t lineaddr)
        :lineaddr_(lineaddr),
        cycle_added_(GL_cycle_)
    {}

    DRAMTransaction(const DRAMTransaction&) =default;
};

struct DRAMTransactionComparator {
    inline bool operator<(const DRAMTransaction* t1, const DRAMTransaction* t2) const {
        return t1->cycle_finished_ > t2->cycle_finished_;
    }
};

using TransactionReturnQueue = std::priority_queue<DRAMTransaction*, 
                                                    std::vector<DRAMTransaction*>,
                                                    DRAMTransactionComparator>;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class DRAMSubchannel {
public:
    constexpr static size_t TRANS_QUEUE_SIZE = 128;
    /*
     * An "opp_write_drain" is an opportunistic write drain
     * that occurs when there are no pending commands to all
     *
     * DRAM banks.
     * */
    uint64_t s_num_opp_write_drains_ =0;
    uint64_t s_num_write_drains_ =0;
    uint64_t s_tot_cycles_between_write_drains_ =0;
    uint64_t s_tot_cycles_between_opp_write_drains_ =0;

    TransactionReturnQueue finished_reads_;

    const DRAMConfig& conf_;
private:
    DRAMRank ranks_[NUM_RANKS];
    size_t next_rank_with_cmd_ =0;
    /*
     * Read management:
     *  `read_queue_` and `pending_reads_` track unfinished reads. If a
     *  `DRAMTransaction` is in the `read_queue_` it has yet to be issued.
     *
     *  `finished_reads_` tracks reads finished by DRAM.
     * */
    std::vector<DRAMTransaction*> read_queue_;
    std::unordered_multimap<uint64_t, DRAMTransaction*> pending_reads_;
    /*
     * Write management: unlike reads, the metadata of `DRAMTransaction` is
     * unnecessary -- we only care about where we are writing.
     * 
     * If `num_writes_to_drain_ > 0`, then the channel switches to
     * write mode and drains `write_buffer_`.
     * */
    std::vector<uint64_t> write_buffer_;
    std::unordered_set<uint64_t> pending_writes_;
    size_t num_writes_to_drain_ =0;

    uint64_t last_drain_cycle_ =0;
    uint64_t last_opp_drain_cycle_ = 0;
    /*
     * Refresh management.
     * */
    uint64_t next_trefi_dram_cycle_ =0;
    size_t next_rank_to_ref_ =0;
public:
    DRAMSubchannel(const DRAMConfig&);

    void tick(void);
    /*
     * Returns true if the request was enqueued.
     * */
    bool make_request(uint64_t lineaddr, bool is_read);
private:
    void schedule_refresh(void);
    void schedule_next_request(void);

    void complete_read(uint64_t, uint64_t latency);
    void complete_write(uint64_t, uint64_t latency);

    bool all_cmd_queues_are_empty(void);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_SUBCHANNEL_h
