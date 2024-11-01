/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_CONFIG_h
#define DRAM_CONFIG_h

#include "defs.h"

#include <string_view>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * Default configuration (assuming an x4 DDR5 32GB DIMM).
 * */
struct DRAMConfig {
    /*
     * Timing (all units but `tCK` are in cycles).
     * */
    double tCK = 0.416;

    size_t CL = 40;
    size_t tRCD = 40;
    size_t tRP = 40;
    size_t tRAS = 77;

    size_t tRFC = 984;
    size_t tREFI = 9390;
    /*
     * Below are auto-populated timing parameters (see
     * `fill_config_xxx` functions below).
     * */
    double itCK;  // inverse of tCK -- to avoid too many divisions.

    size_t CWL;

    size_t tCCD_L;
    size_t tCCD_L_WR; 
    size_t tCCD_L_RTW;
    size_t tCCD_L_WTR;
    size_t tCCD_S;
    size_t tCCD_S_WR;
    size_t tCCD_S_RTW;
    size_t tCCD_S_WTR;

    size_t tRRD_L;
    size_t tRRD_S;

    size_t tFAW;
};

void fill_config_for_4400_4800_5200(DRAMConfig&, std::string_view which="4800");

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_CONFIG_h
