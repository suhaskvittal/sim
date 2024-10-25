/* author: Suhas Vittal
 *  date:   10 October 2024
 * */
#ifndef CORE_h
#define CORE_h

#include "defs.h"

#include <array>
#include <deque>
#include <iostream>
#include <string>

#include <stdint.h>
#include <zlib.h>

constexpr uint64_t BAD_LATENCY = 1'000'000;

/*
 * Class declarations that appear in other files (avoid dependencies).
 * */
class OS;
template <class T> class CacheController;

struct ROBEntry {
    uint64_t inst_num_;

    uint64_t begin_cycle_;
    uint64_t end_cycle_;
};

class Core {
public:
    uint64_t curr_inst_num_ =0;
    uint64_t finished_inst_num_ =0;

    OS* os_;
    /*
     * Microarchitectural structures.
     * */
    CacheController<LLC>* llc_ctrl_;
    ROBEntry rob_[ROB_WIDTH];

    size_t rob_ptr_ =0;
    size_t rob_size_ =0;

    size_t coreid_;
    size_t fetch_width_;

    uint64_t s_tot_delay_ =0;
    uint64_t s_mshr_full_ =0;
    uint64_t s_llc_misses_ =0;
    uint64_t s_llc_accesses_ =0;
private:
    uint64_t inst_num_offset_ =0;
    /*
     * Simulation files.
     * */
    std::string trace_file_;
    gzFile trace_in_;
    /*
     * Instructions are ready from `trace_in_` and placed in this structure. Once the
     * core catches up to this instruction (see `curr_inst_num_`, the instruction is
     * handled.
     * */
    struct {
        uint64_t num;
        uint64_t vla;  // virtual line address
        bool is_wb;
        char linedata[64];
    } next_inst_;
    /*
     * Traces package reads and writes together. However, we want to split them up
     * in case either fails (need to spin on cache controller).
     * */
    struct {
        bool valid=false;
        uint64_t vla;
        char linedata[64];
    } trace_wb_data_;
public:
    Core(void);
    Core(size_t coreid, size_t fetch_width);

    void tick(void);
    void print_stats(std::ostream&);
    void dump_debug_info(std::ostream&);
    /*
     * Sets `trace_in_` and `trace_file_`, and also calls `read_next_inst`.
     * */
    void set_trace_file(std::string);
private:
    void rob_retire(void);

    void read_next_inst(void);
};

#endif  // CORE_h
