/*
 *  author: Suhas Vittal
 *  date:   31 October 2024
 * */

#include "dram/channel.h"

#include <iostream>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DRAMSubchannel::DRAMSubchannel(const DRAMConfig& conf)
    :conf_(conf)
{
    read_queue_.reserve(TRANS_QUEUE_SIZE);
    write_buffer_.reserve(TRANS_QUEUE_SIZE);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMSubchannel::tick() {
    // Check if we need to perform a refresh.
    if (GL_dram_cycle_ >= next_trefi_dram_cycle_) {
        schedule_refresh();
    }
    // Try to issue a command.
    DRAMCommand cmd;
    for (size_t ii = 0; ii < NUM_RANKS; ii++) {
        DRAMRank& rk = ranks_[next_rank_with_cmd_];
        next_rank_with_cmd_ = INCREMENT_AND_MOD_BY_POW2(next_rank_with_cmd_, NUM_RANKS);

        if (rk.select_command(cmd)) {
            uint64_t latency = rk.execute_command(cmd);
            // Update pending results.
            if (cmd.cmd_type == READ_CMD()) complete_read(cmd.lineaddr, latency);
            else                            complete_write(cmd.lineaddr, latency);

            break;
        }
    }

    schedule_next_request();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMSubchannel::make_request(uint64_t lineaddr, bool is_read) {
    if (is_read && read_queue_.size() < TRANS_QUEUE_SIZE) {
        DRAMTransaction* trans = new DRAMTransaction(lineaddr);
        if (pending_writes_.count(lineaddr)) {
            trans->cycle_fired = GL_cycle_;
            trans->cycle_finished = GL_cycle_;
            finished_reads_.push(trans);
        } else {
            read_queue_.push_back(trans);
            pending_reads_.insert(lineaddr, trans);
        }
        return true;
    } else if (!is_read && write_buffer_.size() < TRANS_QUEUE_SIZE) {
        write_buffer_.push_back(lineaddr);
        pending_writes_.insert(lineaddr);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMSubchannel::schedule_refresh() {
    if constexpr (DRAM_REFRESH == DRAMRefreshMethod::REFAB) {
        DRAMRank& rk = ranks_[next_rank_to_ref_];
        rk.set_needs_refresh();
        if ((++next_rank_to_ref_) == NUM_RANKS) {
            next_rank_to_ref_ = 0;
            next_trefi_dram_cycle_ += conf_.tREFI;
        }
    } else {
        std::cerr << "REFsb is currently unsupported!\n";
        exit(1);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMSubchannel::schedule_next_request() {
    bool write_buf_is_full = write_buffer_.size() == TRANS_QUEUE_SIZE,
         cmd_queue_is_empty = all_cmd_queues_are_empty() && write_buffer_.size() > 8;
    // Check if we need to turnaround the bus.
    if (num_writes_to_drain_ == 0 && (write_buf_is_full || cmd_queue_is_empty)) {
        num_writes_to_drain_ = write_buffer_.size();
        // Update stats.
        ++s_num_write_drains_;
        s_tot_cycles_between_write_drains_ += GL_cycle_ - last_drain_cycle_;
        last_drain_cycle_ = GL_cycle_;
        if (cmd_queue_is_empty) {
            ++s_num_opp_write_drains;
            s_tot_cycles_between_opp_write_drains_ += GL_cycle_ - last_opp_drain_cycle_;
            last_opp_drain_cycle_ = GL_cycle_;
        }
    }
    // Now, perform accesses.
    if (num_writes_to_drain_ > 0) {
        for (auto it = write_buffer_.begin(); it != write_buffer_.end(); it++) {
            if (try_and_insert_command<false>(*it)) {
                num_writes_to_drain_ -= 1;
                write_buffer_.erase(it);
                break;
            }
        }
    } else {
        for (auto it = read_queue_.begin(); it != read_queue_.end(); it++) {
            DRAMTransaction* trans = it->second;
            if (try_and_insert_command<true>(trans->lineaddr)) {
                trans->cycle_fired_ = GL_cycle_;
                read_queue_.erase(it);
                break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMSubchannel::complete_read(uint64_t lineaddr, uint64_t latency) {
    while (pending_reads_.count(lineaddr)) {
        auto it = pending_reads_.find(lineaddr);
        DRAMTransaction* trans = it->second;
        trans.cycle_finished_ = GL_cycle_ + latency;

        finished_reads_.push(trans);
        pending_reads_.erase(it);
    }
}

void
DRAMSubchannel::complete_write(uint64_t lineaddr, uint64_t latency) {
    pending_writes_.erase(lineaddr);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMSubchannel::all_cmd_queues_are_empty() {
    bool all_empty = true;
    for (size_t i = 0; i < NUM_RANKS; i++) {
        all_empty &= ranks_[i].all_cmd_queues_are_empty();
    }
    return all_empty;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
