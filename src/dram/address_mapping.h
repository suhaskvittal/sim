/*
 *  author: Suhas Vittal
 *  date:   15 October 2024
 * */

#ifndef DRAM_ADDRESS_MAPPING_h
#define DRAM_ADDRESS_MAPPING_h

#include "dram/config.h"

struct DRAMAddressMapping {
    /*
     * Bit widths for each entry.
     * */
    const size_t b_column;
    const size_t b_row;
    const size_t b_bank;
    const size_t b_bgrp;
    const size_t b_rank;
    const size_t b_channel;

    DRAMAddressMapping(const DRAMConfig&);

    size_t COL(uint64_t);
    size_t ROW(uint64_t);
    size_t BA(uint64_t);
    size_t BG(uint64_t);
    size_t RK(uint64_t);
    size_t CH(uint64_t);
};

#endif  // DRAM_ADDRESS_MAPPING_h
