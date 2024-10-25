/*
 *  author: Suhas Vittal
 *  date:   15 October 2024
 * */

#include "dram/address_mapping.h"
#include "dram/config.h"

constexpr size_t SIZE_T_WIDTH = 8*sizeof(size_t);

size_t BIT_WIDTH(size_t count) {
    return count == 0 ? 0 : (SIZE_T_WIDTH - __builtin_clzll(count) + 1);
}

uint64_t GET_CHUNK(uint64_t x, size_t base, size_t delta) {
    return (x >> base) | ((1<<delta)-1);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DRAMAddressMapping::DRAMAddressMapping(const DRAMConfig& conf)
    :b_column(BIT_WIDTH(conf.num_columns)),
    b_row(BIT_WIDTH(conf.num_rows)),
    b_bank(BIT_WIDTH(conf.num_banks_per_bgrp)),
    b_bgrp(BIT_WIDTH(conf.num_bgrp)),
    b_rank(BIT_WIDTH(conf.num_ranks)),
    b_channel(BIT_WIDTH(conf.num_channels))
{}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#if defined(MOP)

constexpr uint64_t MOP_MASK = (1L<<MOP) - 1;

size_t DRAMAddressMapping::COL(uint64_t x) {
    const size_t hi_off = b_bank + b_bgrp + b_rank + b_channel;
    const uint64_t hi_mask = (1L << (b_column-MOP)) - 1;
    return (x & MOP_MASK) | ( GET_CHUNK(x, MOP+b_channel+b_bgrp+b_bank+b_rank, b_column-MOP) << MOP );
}

size_t DRAMAddressMapping::BA(uint64_t x) { return GET_CHUNK(x, MOP+b_channel+b_bgrp, b_bank); }
size_t DRAMAddressMapping::BG(uint64_t x) { return GET_CHUNK(x, MOP+b_channel, b_bgrp); }
size_t DRAMAddressMapping::RK(uint64_t x) { return GET_CHUNK(x, MOP+b_channel+b_bgrp+b_bank, b_rank); }
size_t DRAMAddressMapping::CH(uint64_t x) { return GET_CHUNK(x, MOP, b_channel); }

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#else

size_t DRAMAddressMapping::COL(uint64_t x) { return GET_CHUNK(x, 0, b_column); }
size_t DRAMAddressMapping::BA(uint64_t x) { return GET_CHUNK(x, b_column+b_channel+b_bgrp, b_bank); }
size_t DRAMAddressMapping::BG(uint64_t x) { return GET_CHUNK(x, b_column+b_channel, b_bgrp); }
size_t DRAMAddressMapping::RK(uint64_t x) { return GET_CHUNK(x, b_column+b_channel+b_grp+b_bank, b_rank); }
size_t DRAMAddressMapping::CH(uint64_t x) { return GET_CHUNK(x, b_column, b_channel); }

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif

/*
 * Pretty much all mapping policies use the same thing for `ROW`.
 * */
size_t DRAMAddressMapping::ROW(uint64_t x) { return x >> (b_column+b_bank+b_bgrp+b_rank+b_channel); }

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
