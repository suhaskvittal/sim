/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#include "defs.h"
#include "dram/config.h"

#include <algorithm>

#include <math.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline int CKCAST(DRAMConfig& conf, double time_ns) {
    return static_cast<int>( round(time_ns * conf.itCK) ); 
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline void
fill_common(DRAMConfig& conf) {
    conf.itCK = 1.0 / conf.tCK;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
fill_config_for_4400_4800_5200(DRAMConfig& conf, std::string_view which) {
    fill_common(conf);

    conf.CWL = conf.CL-2;

    conf.tCCD_L = std::max( 8, CKCAST(conf,5) );
    conf.tCCD_L_WR = std::max( 32, CKCAST(conf,20) );
    conf.tCCD_L_WTR = conf.CWL + BURST_LENGTH + std::max(16, CKCAST(conf,10));
    conf.tCCD_L_RTW = conf.tCCD_L_WTR;

    conf.tCCD_S = 8;
    conf.tCCD_S_WR = 8;
    conf.tCCD_S_WTR = conf.CWL = BURST_LENGTH + std::max(4, CKCAST(conf, 2.5));
    conf.tCCD_S_RTW = conf.tCCD_S_WTR;

    conf.tRRD_L = std::max( 8, CKCAST(conf, 5) );
    conf.tRRD_S = 8;

    if (which == "4400")        conf.tFAW = std::max(32, CKCAST(conf, 14.545));
    else if (which == "4800")   conf.tFAW = std::max(32, CKCAST(conf, 13.333));
    else                        conf.tFAW = std::max(40, CKCAST(conf, 15.384));
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
