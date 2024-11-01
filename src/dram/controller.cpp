/*
 *  author: Suhas Vittal
 *  date:   31 October 2024
 * */

#include "defs.h"
#include "cache/controller/llc2.h"
#include "dram/controller.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

static const double DRAM_CLOCK_SCALE = 4.0/2.4 - 1.0;

uint64_t GL_dram_cycle_ = 0;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DRAMController::DRAMController() {}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMController::tick() {
    bool tick_mem = leap_op_ < 1.0;
    for (size_t i = 0; i < N_MEM; i++) {
        if (tick_mem) mem_[i].tick();
        // Check if any requests have finished.
        while (mem_[i].finished_reads_.size() > 0) {
            DRAMTransaction* trans = mem_[i].finished_reads_.top();
            if (GL_dram_cycle_ < trans->dram_cycle_finished_) {
                break; 
            }
            mem_[i].finished_reads_.pop();
            // Update LLC and stats.
            GL_llc_controller_->mark_as_finished(trans->lineaddr_);
            s_tot_read_latency_ += GL_cycle_ - trans->cpu_cycle_added_;

            delete trans;
        }
    }
    if (tick_mem) {
        leap_op_ += DRAM_CLOCK_SCALE;
        ++GL_dram_cycle_;
    } else {
        leap_op_ -= 1.0;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

bool
DRAMController::make_request(uint64_t lineaddr, bool is_read) {
    size_t idx = CHANNEL(lineaddr) * NUM_SUBCHANNELS + SUBCHANNEL(lineaddr);
    if (is_read) ++s_num_reads_;
    else         ++s_num_writes_;
    return mem_[idx].make_request(lineaddr, is_read);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
DRAMController::print_stats(std::ostream& out) {
    double mean_read_latency = ((double)s_tot_read_latency_)/((double)s_num_reads_);

    PRINT_STAT(out, "DRAM_READS", s_num_reads_);
    PRINT_STAT(out, "DRAM_WRITES", s_num_writes_);
    PRINT_STAT(out, "DRAM_READ_LATENCY", mean_read_latency);

    DRAMSubchannelStats sc_stats;
    for (size_t i = 0; i < N_MEM; i++) {
        mem_[i].accumulate_stats_into(sc_stats);
    }
    sc_stats.print_stats(out);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
