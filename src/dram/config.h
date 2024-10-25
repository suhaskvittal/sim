/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_CONFIG_h
#define DRAM_CONFIG_h

#include "defs.h"

#include <string>

enum class PagePolicy { OPEN, CLOSED };
enum class RefreshPolicy { RANK_CONCURRENT, RANK_STAGGERED };
/*
 * Default configuration (assuming an x4 DDR5 32GB DIMM).
 * */
struct DRAMConfig {
    /*
     * Organization. Note:
     *  (1) `num_bgrps`, `num_banks_per_brp` are per rank.
     *  (2) `num_rows` is per bank.
     *  (3) `columns` is per-chip. `columns` x `device_width` x `#-of-chips-per-rank` is the 
     *      size of a row.
     * */
    size_t channel_size_mb = 1L << 34;
    size_t num_bgrps = 8;
    size_t num_banks_per_bgrp = 4;
    size_t num_rows = 4194304;
    size_t num_columns = 1024;
    size_t device_width = 4;
    
    size_t bus_width = 32;
    size_t num_channels = 2;

    PagePolicy page_policy = PagePolicy::OPEN;
    RefreshPolicy ref_policy = RefreshPolicy::RANK_STAGGERED;
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
    size_t num_banks;
    size_t num_ranks;
    size_t burst_length;

    double itCK;  // inverse of tCK -- to avoid too many divisions.

    size_t CWL;

    size_t tCCD_L;
    size_t tCCD_M;
    size_t tCCD_L_WR; 
    size_t tCCD_M_WR;
    size_t tCCD_L_RTW;
    size_t tCCD_L_WTR;
    size_t tCCD_M_WTR;
    size_t tCCD_S;
    size_t tCCD_S_WR;
    size_t tCCD_S_RTW;
    size_t tCCD_S_WTR;

    size_t tRRD_L;
    size_t tRRD_S;

    size_t tFAW;
    /*
     * Unused.
     * */
    size_t tCCD_L_WR2;
};

void fill_config_for_4400_4800_5200(DRAMConfig&, std::string which="4800");

#endif  // DRAM_CONFIG_h
