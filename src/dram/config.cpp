/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#include "dram/config.h"

#include <algorithm>

#include <math.h>

inline size_t CKCAST(DRAMConfig& conf, double time_ns) {
    return static_cast<size_t>( round(time_ns * conf.itCK) ); 
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
fill_common(DRAMConfig& conf) {
    conf.num_banks = conf.num_bgrps*conf.num_banks_per_bgrp;

    size_t row_size = (conf.num_columns * conf.device_width * (conf.bus_width/conf.device_width)) / 8;
    size_t rank_size_mb = row_size*conf.num_banks;

    conf.num_ranks = conf.channel_size_mb/rank_size_mb;
    conf.burst_length = 64 / conf.device_width;

    // Timing commons:
    conf.itCK = 1.0 / conf.tCK;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
fill_config_for_4400_4800_5200(DRAMConfig& conf, std::string which) {
    conf.CWL = conf.CL-2;

    conf.tCCD_L = std::max( 8, CKCAST(conf,5) );
    conf.tCCD_M = conf.tCCD_L;
    conf.tCCD_L_WR = std::max( 32, CKCAST(conf,20) );
    conf.tCCD_M_WR = conf.tCCD_L_WR;
    conf.tCCD_L_WTR = conf.CWL + (conf.burst_length/2) + std::max(16, CKCAST(conf,10));
    conf.tCCD_L_RTW = conf.tCCD_L_WTR;

    conf.tCCD_S = 8;
    conf.tCCD_S_WR = 8;
    conf.tCCD_S_WTR = conf.CWL = (conf.burst_length/2) + std::max(4, CKCAST(conf, 2.5));
    conf.tCCD_S_RTW = conf.tCCD_S_WTR;

    conf.tRRD_L = std::max( 8, CKCAST(conf, 5) );
    conf.tRRD_S = 8;

    if (which == "4400")        conf.tFAW = std::max(32, CKCAST(conf, 14.545));
    else if (which == "4800")   conf.tFAW = std::max(32, CKCAST(conf, 13.333));
    else                        conf.tFAW = std::max(40, CKCAST(conf, 15.384));
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
