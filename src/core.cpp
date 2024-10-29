/*
 *  author: Suhas Vittal
 *  date:   10 October 2024
 * */

#include "core.h"
#include "cache/controller/llc2.h"
#include "os.h"
#include <iostream>

#include <stdlib.h>
#include <string.h>

Core::Core()
    :Core(0,1)
{}

Core::Core(size_t coreid, size_t fw)
    :coreid_(coreid),
    fetch_width_(fw)
{}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::tick() {
    // Try and retire ROB entries.
    rob_retire();

    size_t robid = (rob_ptr_+rob_size_) & (ROB_WIDTH-1);
    for (size_t i = 0; i < fetch_width_ && rob_size_ < ROB_WIDTH; i++) {
        // Setup ROB entry early. If we end up not using it, no harm, no foul.
        rob_[robid] = { curr_inst_num_, GL_cycle_, GL_cycle_ };
        if (curr_inst_num_ >= next_inst_.num) {
            bool is_load = !next_inst_.is_wb;
            uint64_t lineaddr = GL_os_->v2p( next_inst_.vla );
            if (is_load) { // Need to wait for access to finish.
                rob_[robid].end_cycle_ = GL_cycle_ + BAD_LATENCY;
            }
            int retval = GL_llc_controller_->access(lineaddr, coreid_, robid, curr_inst_num_, is_load);
            if (retval == -1) {
                ++s_mshr_full_;
                return;
            } else {
                ++s_llc_accesses_;
                if (retval == 0) {
                    ++s_llc_misses_;
                }
            }
            read_next_inst();
        }
        ++rob_size_;
        robid = INCREMENT_AND_MOD_BY_POW2(robid, ROB_WIDTH);
        
        ++curr_inst_num_;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::print_stats(std::ostream& out) {
    std::string header = "CORE_" + std::to_string(coreid_);

    double ipc = ((double)finished_inst_num_)/((double)GL_cycle_);
    double mpki = 1000*((double)s_llc_misses_)/((double)finished_inst_num_);
    double apki = 1000*((double)s_llc_accesses_)/((double)finished_inst_num_);
    double delay = ((double)s_tot_delay_)/((double)finished_inst_num_);

    PRINT_STAT(out, header + "_INST", finished_inst_num_);
    PRINT_STAT(out, header + "_IPC", ipc);
    PRINT_STAT(out, header + "_MISSES", s_llc_misses_);
    PRINT_STAT(out, header + "_ACCESSES", s_llc_accesses_);
    PRINT_STAT(out, header + "_MPKI", mpki);
    PRINT_STAT(out, header + "_APKI", apki);
//  PRINT_STAT(out, header + "_SLEEP", s_mshr_full_);
//  PRINT_STAT(out, header + "_DELAY", delay);
    out << "\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::dump_debug_info(std::ostream& out) {
    out << "CORE " << coreid_ << " DUMP --------------------------------------------\n";
    out << "\tCurrent Inst = " << curr_inst_num_ << "\n";
    out << "\tFinished Inst = " << finished_inst_num_ << "\n";
    out << "\tROB:\n";
    for (size_t i = 0; i < rob_size_; i++) {
        size_t ii = (rob_ptr_+i) % ROB_WIDTH;
        ROBEntry& e = rob_[ii];
        out << "\t\t[ " << ii << " ] inst_num " << e.inst_num_
            << ", begin = " << e.begin_cycle_
            << ", end = " << e.end_cycle_ << "\n";
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::set_trace_file(std::string f) {
    trace_file_ = f;
    trace_in_ = gzopen(f.c_str(), "r");
    read_next_inst();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::rob_retire() {
    for (size_t i = 0; i < fetch_width_ && rob_size_ > 0; i++) {
        ROBEntry& e = rob_[rob_ptr_];
        if (e.end_cycle_ <= GL_cycle_) {
            --rob_size_;
            rob_ptr_ = INCREMENT_AND_MOD_BY_POW2(rob_ptr_, ROB_WIDTH);
            finished_inst_num_ = e.inst_num_;
#ifdef DEBUG_CORE
            std::cout << "[ debug core " << coreid_ << " ] inst " << e.inst_num_
                << " | retired @ cycle " << GL_cycle_ << "\n";
#endif
            s_tot_delay_ += e.end_cycle_ - e.begin_cycle_;
        } else break; // cannot proceed.
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
Core::read_next_inst() {
    if (gzeof(trace_in_)) {
        gzclose(trace_in_);
        // Reread trace file.
        trace_in_ = gzopen(trace_file_.c_str(), "r");
        // Reset `curr_inst_num_`, and update `inst_num_offset_`.
        inst_num_offset_ = curr_inst_num_;
        curr_inst_num_ = 0;
    }
#ifdef COMPRESSION_TRACES
    // If we already have WB data, just use that.
    if (trace_wb_data_.valid) {
        trace_wb_data_.valid = false;
        next_inst_.vla = trace_wb_data_.vla;
        next_inst_.is_wb = true;
        memmove( next_inst_.linedata, trace_wb_data_.linedata, 64 );
    } else {
        gzread( trace_in_, &next_inst_.num, 8 );
        gzseek( trace_in_, 1, SEEK_CUR);
        gzread( trace_in_, &next_inst_.vla, 8 );

        next_inst_.is_wb = false;

        gzread( trace_in_, &trace_wb_data_.valid, 1 );
        if (trace_wb_data_.valid) {
            gzread( trace_in_, &trace_wb_data_.vla, 8 );
            gzread( trace_in_, trace_wb_data_.linedata, 64 );
        }

    }
    next_inst_.vla >>= Log2<LINESIZE>::value; // Note that the given addresses are virtual byte addresses,
                                                  // not LINE addresses.
#else
    gzread( trace_in_, &next_inst_.num, 5 );
    gzread( trace_in_, &next_inst_.is_wb, 1 );
    gzread( trace_in_, &next_inst_.vla, 4);
#endif
    TAG_VA_WITH_COREID(next_inst_.vla, coreid_);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

