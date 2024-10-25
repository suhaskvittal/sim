/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#include "dram/channel.h"

#include <strings.h>

DRAMChannel::DRAMChannel(const DRAMConfig& conf)
    :num_bgrps_(conf.num_bgrps / conf.num_channels),
    ranks_(conf.num_ranks, RankData(conf)),
    conf_(conf)
{
    if (conf.ref_policy == RefreshPolicy::RANK_STAGGERED) {
        // Offset each rank's tREFI cycle.
        uint64_t off = conf.tREFI / conf.num_ranks;
        for (size_t i = 0; i < conf.num_ranks, i++) {
            ranks_[i].next_trefi_cycle_ = off * i;
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMChannel::tick() {
    manage_refresh();
    retire_next_cmd();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMChannel::push_back_cmd(CmdQueueEntry&& e) {
    if (cmd_queue_size_ == CMD_QUEUE_SIZE) return false;    
    size_t i = (cmd_queue_ptr_ + cmd_queue_size_) & (CMD_QUEUE_SIZE-1);
    
    e.t_entered_queue_ = cycle_;

    cmd_queue_[i] = e;
    cmd_queue_ptr_ = (cmd_queue_ptr_+1) & (CMD_QUEUE_SIZE-1);
    ++cmd_queue_size_;
    return true;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
void
DRAMChannel::check_faw() {
    for (RankData& rk : ranks_) {
        for (auto it = rk.faw_cycles_.begin(); it != rk.faw_cycles_.end(); ) {
            if (cycle_ >= *it) it = rk.faw_cycles_.erase(it);
            else break;
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMChannel::manage_refresh() {
    for (RankData& rk : ranks_) {
        if (cycle_ < rk.next_trefi_cycle_) continue;

        size_t bankid;
        while ((bankid=ffsll(rk.banks_done_with_curr_trefi_)) < conf_.num_banks) {
            size_t bgrp = bankid / conf_.num_banks_per_bgrp,
                   bank = bankid % conf_.num_banks_per_bgrp;
            DRAMBank& bank = rk.banks_[bgrp][bank];
            CmdQueueEntry ref = { CmdType::REFRESH };
            if (bank.do_cmd(ref)) {
                rk.banks_done_with_curr_trefi_ &= ~(1L << bankid);
            }
        }
        // Check if all banks are done with refresh.
        if (rk.banks_done_with_curr_trefi_ == 0) {
            rk.banks_done_with_curr_trefi_ = ~0;
            rk.next_trefi_cycle_ += conf_.tREFI;
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMChannel::execute_next_cmd() {
    if (cmd_queue_size_ == 0) return;

    CmdQueueEntry& e = cmd_queue_[cmd_queue_ptr_];
    bool is_write = e.cmd_type == CmdType::WRITE;
    // Get appropriate bank.
    RankData& rk = ranks_[e.rank_];
    // Check if command violates timing constraints.
    size_t c;
    if (rk.last_bgrp_ == e.bgrp_) {
        if (rk.last_bank_ = e.bank_) {
            if (rk.last_bank_was_written_to_) {
                c = is_write ? conf_.tCCD_L_WR : conf_.tCCD_L_WTR;
            } else {
                c = is_write ? conf_.tCCD_L_RTW : conf_.tCCD_L;
            }
        } else {
            if (last_bank_was_written_to_) {
                c = is_write ? conf_.tCCD_M_WR : conf_.tCCD_M_WTR;
            } else {
                c = is_write ? conf_.tCCD_L_RTW : conf_.tCCD_L;
            }
        }
    } else {
        if (rk.last_bank_was_written_to_) {
            c = is_write ? tCCD_S_WR : tCCD_S_WTR;
        } else {
            c = is_write ? tCCD_S_RTW : tCCD_S;
        }
    }
    if (cycle_ < rk.last_bank_cycle_ + c) return;
    // Now, we know that the constraint can be completed. Now, check
    // if the bank is available.
    DRAMBank& bank = rk.banks_[e.bgrp_][e.bank_];
    if (cycle_ < bank.busy_until_cycle_) return;
    // Check if tFAW is being violated.
    bool is_act = !bank.is_row_in_row_buffer(e.row_);
    if (is_act && rk.faw_cycles_.size() == 4) return;
    // Bank is available. Update `e` and execute command.
    e.t_exited_queue_ = cycle_;
    bank.do_cmd(e);
    // Update FAW.
    if (is_act) rk.faw_cycles_.push_back(cycle_ + conf.tFAW);
    // Update rank data.
    rk.last_bgrp_ = e.bgrp_;
    rk.last_bank_ = e.bank_;
    rk.last_bank_cycle_ = cycle_;
    rk.last_bank_was_written_to_ = is_write;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

