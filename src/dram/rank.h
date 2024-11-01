/*
 *  author: Suhas Vittal
 *  date:   30 October 2024
 * */

#ifndef DRAM_RANK_h
#define DRAM_RANK_h

#include "defs.h"
#include "dram/address.h"
#include "dram/bank.h"

#include <deque>
#include <unordered_set>
#include <vector>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct DRAMCommand {
    uint64_t lineaddr_;
    DRAMCommandType cmd_type_;
};

inline constexpr DRAMCommandType READ_CMD() {
    if constexpr (DRAM_PAGE_POLICY == DRAMPagePolicy::OPEN) return DRAMCommandType::READ;
    else                                                    return DRAMCommandType::READ_PRECHARGE;
}

inline constexpr DRAMCommandType WRITE_CMD() {
    if constexpr (DRAM_PAGE_POLICY == DRAMPagePolicy::OPEN) return DRAMCommandType::WRITE;
    else                                                    return DRAMCommandType::WRITE_PRECHARGE;
}

using CommandQueue = std::vector<DRAMCommand>;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class DRAMRank {
public:
    constexpr static size_t CMD_QUEUE_SIZE = 32;
    constexpr static size_t N_CMD_QUEUES = NUM_BANKGROUPS*NUM_BANKS;

    DRAMBank banks_[NUM_BANKGROUPS][NUM_BANKS];
    size_t rankid_;

    uint64_t s_num_read_cmds_ = 0;
    uint64_t s_num_write_cmds_ = 0;
    uint64_t s_num_acts_ =0;
    uint64_t s_num_pre_ =0;
    uint64_t s_num_pre_demand_ =0;
    uint64_t s_row_buf_hits_ =0;
private:
    std::unordered_set<uint64_t> lineaddr_with_recent_row_miss_;

    CommandQueue cmd_queues_[N_CMD_QUEUES];
    size_t next_cmd_queue_idx_ =0;
    uint64_t num_cmds_ =0;
    /*
     * `last_four_act_cycles_` enforces tFAW: if the 4-ACT window is full
     * (size of `deque` is 4), then the rank does not accept any more ACTs.
     *
     * The bankgroup accessed last is tracked in `last_bankgroup_used_`. This
     * is used to enforce the following constraints.
     *   (i) tRRD is enforced by `next_row_access_ok_cycle_`
     *   (ii) tCCD, tCCD_WR, tCCD_RTW, and tCCD_WTR are enforced by
     *      `next_column_read_ok_cycle_` and `next_column_write_ok_cycle_`.
     * */
    std::deque<uint64_t> last_four_act_dram_cycles_;
    size_t last_bankgroup_used_ =0;
    uint64_t next_row_activate_ok_cycle_[2];
    uint64_t next_column_read_ok_cycle_[2];
    uint64_t next_column_write_ok_cycle_[2];

    bool is_waiting_to_do_ref_ =false;
    /*
     * This is to check if any bank is performing an operation.
     * */
    uint64_t any_bank_busy_until_dram_cycle_ =0;
public:
    DRAMRank(void);
    
    void tick(void);

    void set_needs_refresh(void);
    /*
     * Trys to insert a command (read/write) for `lineaddr`. Depending on the
     * page policy, this is either `READ` or `READ_PRECHARGE` (or the corresponding
     * WRITE versions if `IS_READ == false`).
     *
     * Returns true if the command was inserted.
     * */
    template <bool IS_READ>
    bool try_and_insert_command(uint64_t lineaddr);
    /*
     * Returns true if a command was found (so `DRAMCommand&` will point to a valid command).
     *
     * If a command is dependent on another command to execute (i.e. a READ to a row requiring
     * an ACTIVATE first), then the required command is given instead and no command is removed
     * from any command queue.
     * */
    bool select_command(DRAMCommand&);
    /*
     * Returns true if the command meets all DRAM timing constraints.
     * */
    bool can_execute_command(const DRAMCommand&);
    /*
     * Returns the latency of the executed command.
     * */
    uint64_t execute_command(const DRAMCommand&);
    /*
     * Returns reference to corresponding command queue.
     * */
    CommandQueue& get_command_queue(size_t bg, size_t ba);
    bool all_cmd_queues_are_empty(void);
private:
    void issue_refresh(void);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline void DRAMRank::set_needs_refresh() { is_waiting_to_do_ref_ = true; }

template <bool IS_READ> bool
DRAMRank::try_and_insert_command(uint64_t lineaddr) {
    constexpr DRAMCommandType CMD_TYPE = IS_READ ? READ_CMD() : WRITE_CMD();

    size_t bg = BANKGROUP(lineaddr),
           ba = BANK(lineaddr);
    auto& cq = get_command_queue(bg, ba);
    if (cq.size() < CMD_QUEUE_SIZE) {
        cq.push_back((DRAMCommand) {lineaddr, CMD_TYPE});
        ++num_cmds_;
        return true;
    } else {
        return false;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_RANK_h
