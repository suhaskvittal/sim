/* author: Suhas Vittal
 *  date:   31 October 2024
 * */

#include "dram/rank.h"
#include "utils/bitcount.h"

#include <string.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

constexpr size_t BL = BURST_LENGTH;

static size_t rankcnt = 0;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DRAMRank::DRAMRank() 
    :rankid_(rankcnt++)
{
    for (size_t i = 0; i < N_CMD_QUEUES; i++) {
        cmd_queues_[i].reserve(CMD_QUEUE_SIZE);
    }

    memset(next_row_activate_ok_cycle_, 0, 2*sizeof(uint64_t));
    memset(next_column_read_ok_cycle_, 0, 2*sizeof(uint64_t));
    memset(next_column_write_ok_cycle_, 0, 2*sizeof(uint64_t));
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMRank::tick() {
    // Check if we need to do a refresh.
    if (is_waiting_to_do_ref_) {
        if (GL_dram_cycle_ >= any_bank_busy_until_dram_cycle_) {
            issue_refresh();
            is_waiting_to_do_ref_ = false;
        }
    }
    // Update FAW.
    while (!last_four_act_dram_cycles_.empty() 
            && GL_dram_cycle_ >= last_four_act_dram_cycles_.front() + GL_dram_conf_.tFAW) 
    {
        last_four_act_dram_cycles_.pop_front();
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMRank::select_command(DRAMCommand& cmd) {
    if (is_waiting_to_do_ref_) return false;

    for (size_t ii = 0; ii < N_CMD_QUEUES; ii++) {
        size_t ba = next_cmd_queue_idx_ & (NUM_BANKS-1),
               bg = next_cmd_queue_idx_ >> Log2<NUM_BANKS>::value;
        DRAMBank& bank = banks_[bg][ba];
        CommandQueue& cq = cmd_queues_[next_cmd_queue_idx_];
        next_cmd_queue_idx_ = INCREMENT_AND_MOD_BY_POW2(next_cmd_queue_idx_, CMD_QUEUE_SIZE);

        if (GL_dram_cycle_ < bank.busy_with_ref_until_dram_cycle_ || cq.empty()) {
            continue;
        }
        
        // First, try and search for row buffer hits.
        for (auto it = cq.begin(); it != cq.end(); it++) {
            uint64_t r = ROW(it->lineaddr_);
            if (r == bank.open_row_ && can_execute_command(*it)) {
                // We have found our candidate.
                cmd = *it;
                cq.erase(it);
                --num_cmds_;

                if (!lineaddr_with_recent_row_miss_.count(it->lineaddr_)) {
                    ++s_row_buf_hits_;
                }
                lineaddr_with_recent_row_miss_.erase(it->lineaddr_);
                return true;
            }
        }

        // No row buffer hits: now do FCFS.
        for (auto it = cq.begin(); it != cq.end(); it++) {
            if (ROW(it->lineaddr_) == bank.open_row_) continue;
            cmd.lineaddr_ = it->lineaddr_;
            cmd.cmd_type_ = (bank.open_row_ >= 0) ? DRAMCommandType::PRECHARGE : DRAMCommandType::ACTIVATE;

            // Return now if we can execute this command; otherwise, keep searching.
            if (can_execute_command(cmd)) {
                lineaddr_with_recent_row_miss_.insert(cmd.lineaddr_);
                return true;
            }
        }
    } 
    return false;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMRank::can_execute_command(const DRAMCommand& cmd) {
    DRAMBank& bank = banks_[ BANKGROUP(cmd.lineaddr_) ][ BANK(cmd.lineaddr_) ];
    if (GL_dram_cycle_ < bank.busy_with_ref_until_dram_cycle_) {
        return false;
    }
    // Get same-bankgroup/diff-bankgroup index. 
    size_t sbg = (last_bankgroup_used_ == BANKGROUP(cmd.lineaddr_)) ? 1 : 0;
    // Check if tCCD timings are met:
    bool read_is_ok = GL_dram_cycle_ >= next_column_read_ok_cycle_[sbg];
    bool write_is_ok = GL_dram_cycle_ >= next_column_write_ok_cycle_[sbg];
    // Check if row buffer contains row and tCCD/tRCD timings are met.
    bool read_write_is_ok = 
            bank.open_row_ == ROW(cmd.lineaddr_) 
                && GL_dram_cycle_ >= bank.next_column_access_ok_cycle_
                && ( (cmd.cmd_type_ == DRAMCommandType::READ && read_is_ok) 
                        || 
                     (cmd.cmd_type_ == DRAMCommandType::WRITE && write_is_ok) );
    // Check if tRAS timing is met
    bool precharge_is_ok = 
            bank.open_row_ >= 0 && GL_dram_cycle_ >= bank.next_precharge_ok_cycle_;
    // Check if tRP/tRRD/tFAW timing is met
    bool activate_is_ok = 
            bank.open_row_ == -1 
                && GL_dram_cycle_ >= bank.next_activate_ok_cycle_ 
                && GL_dram_cycle_ >= next_row_activate_ok_cycle_[sbg]
                && last_four_act_dram_cycles_.size() < 4;
    // Now return based on above:
    switch (cmd.cmd_type_) {
        case DRAMCommandType::READ:
        case DRAMCommandType::WRITE:
            return read_write_is_ok;

        case DRAMCommandType::PRECHARGE:
            return precharge_is_ok;

        case DRAMCommandType::ACTIVATE:
            return activate_is_ok;

        case DRAMCommandType::READ_PRECHARGE:
        case DRAMCommandType::WRITE_PRECHARGE:
            return read_write_is_ok && precharge_is_ok;

        default:
            return true;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline void
update_timing(uint64_t* con, uint64_t t_s, uint64_t t_l) {
    con[0] = GL_dram_cycle_ + t_s;
    con[1] = GL_dram_cycle_ + t_l;
}

uint64_t
DRAMRank::execute_command(const DRAMCommand& cmd) {
    DRAMBank& bank = banks_[ BANKGROUP(cmd.lineaddr_) ][ BANK(cmd.lineaddr_) ];
    // We are assuming all timing constraints have been met.
    uint64_t latency = 0;
    switch (cmd.cmd_type_) {
        case DRAMCommandType::READ_PRECHARGE:
            bank.open_row_ = -1;
            bank.next_activate_ok_cycle_ = GL_dram_cycle_ + GL_dram_conf_.tRP;
            ++s_num_pre_;
        case DRAMCommandType::READ:
            latency += GL_dram_conf_.CL + BL/2;
            // Update timing constraints.
            update_timing(next_column_read_ok_cycle_, GL_dram_conf_.tCCD_S, GL_dram_conf_.tCCD_L);
            update_timing(next_column_write_ok_cycle_, GL_dram_conf_.tCCD_S_RTW, GL_dram_conf_.tCCD_L_RTW);
            ++s_num_read_cmds_;
            break;

        case DRAMCommandType::WRITE_PRECHARGE:
            bank.open_row_ = -1;
            bank.next_activate_ok_cycle_ = GL_dram_cycle_ + GL_dram_conf_.tRP;
            ++s_num_pre_;
        case DRAMCommandType::WRITE:
            latency += GL_dram_conf_.CWL + BL/2;
            // Update timing constraints.
            update_timing(next_column_read_ok_cycle_, GL_dram_conf_.tCCD_S_WTR, GL_dram_conf_.tCCD_L_WTR);
            update_timing(next_column_write_ok_cycle_, GL_dram_conf_.tCCD_S_WR, GL_dram_conf_.tCCD_L_WR);
            ++s_num_write_cmds_;
            break;

        case DRAMCommandType::PRECHARGE:
            bank.open_row_ = -1;
            latency += GL_dram_conf_.tRP;
            // Update timing constraints.
            bank.next_activate_ok_cycle_ = GL_dram_cycle_ + GL_dram_conf_.tRP;
            // Update stats.
            ++s_num_pre_;
            break;

        case DRAMCommandType::ACTIVATE:
            bank.open_row_ = ROW(cmd.lineaddr_);
            latency += GL_dram_conf_.tRCD;
            // Update timing constraints.
            bank.next_precharge_ok_cycle_ += GL_dram_conf_.tRAS;
            bank.next_column_access_ok_cycle_ += GL_dram_conf_.tRCD;
            update_timing(next_row_activate_ok_cycle_, GL_dram_conf_.tRRD_S, GL_dram_conf_.tRRD_L);
            last_four_act_dram_cycles_.push_back(GL_dram_cycle_);
            // Update stats.
            ++s_num_acts_;
            break;

        case DRAMCommandType::REFRESH:
            std::cerr << "DRAMRank::execute_command should not issue refreshes! Use DRAMRank::issue_refresh instead!\n";
            exit(1);
    }
    any_bank_busy_until_dram_cycle_ = std::max(any_bank_busy_until_dram_cycle_, GL_dram_cycle_ + latency);
    last_bankgroup_used_ = BANKGROUP(cmd.lineaddr_);
    return latency;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMRank::issue_refresh() {
    for (size_t i = 0; i < NUM_BANKGROUPS; i++) {
        for (size_t j = 0; j < NUM_BANKS; j++) {
            banks_[i][j].busy_with_ref_until_dram_cycle_ = GL_dram_cycle_ + GL_dram_conf_.tRFC;
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMRank::all_cmd_queues_are_empty() {
    return num_cmds_ == 0;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CommandQueue&
DRAMRank::get_command_queue(size_t bg, size_t ba) {
    return cmd_queues_[ bg*NUM_BANKS + ba ];
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
