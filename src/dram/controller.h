/*
 *  author: Suhas Vittal
 *  date:   31 October 2024
 * */

#ifndef DRAM_CONTROLLER_h
#define DRAM_CONTROLLER_h

#include "dram/subchannel.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class DRAMController {
public:
    constexpr static size_t N_MEM = NUM_CHANNELS*NUM_SUBCHANNELS;

    uint64_t s_num_reads_ =0;
    uint64_t s_num_writes_ =0;
    uint64_t s_tot_read_latency_ =0;
private:
    DRAMSubchannel mem_[N_MEM];

    double leap_op_ =0.0;
public:
    DRAMController(void);

    void tick(void);
    bool make_request(uint64_t lineaddr, bool is_read);

    void print_stats(std::ostream&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_CONTROLLER_h
