/*
 *  author: Suhas Vittal
 *  date:   14 October 2024
 * */

#ifndef DRAM_BANK_h
#define DRAM_BANK_h

#include "defs.h"
#include "dram/config.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct DRAMBank {
    constexpr static size_t MAX_CONSECUTIVE_COLUMN_ACCESSES = 4;
    /*
     * `open_row_ == -1` means no row is in the global row buffer.
     * */
    int64_t open_row_ =-1;

    uint64_t busy_with_ref_until_dram_cycle_ =0;
    /*
     * Row should be precharged after `MAX_CONSECTIVE_COLUMN_ACCESSES` is hit,
     * provided that the bank has no other outstanding accesses to the open row.
     * */
    size_t consecutive_column_accesses_ =0;
    /*
     * Timing constraint handling:
     *  `next_precharge_ok_cycle_` is tRAS after an ACT
     *  `next_activate_ok_cycle_` is tRP after a PRE
     *  `next_column_access_ok_cycle_` is tRCD after an ACT
     *
     * Note that there are other rank level constraints (see `rank.h`).
     * */
    uint64_t next_precharge_ok_cycle_       =0;
    uint64_t next_activate_ok_cycle_        =0;
    uint64_t next_column_access_ok_cycle_   =0;
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_BANK_h
