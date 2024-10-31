/*
 *  author: Suhas Vittal
 *  date:   30 October 2024
 * */

#ifndef DRAM_ADDRESS_h
#define DRAM_ADDRESS_h

#include "defs.h"
#include "utils/bitcount.h"

#include <stdint.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * The following should be inlined functions that quickly
 * compute the given data given a line address.
 *
 * The definitions are the specified *.inl files.
 * */
uint64_t CHANNEL(uint64_t);
uint64_t SUBCHANNEL(uint64_t);
uint64_t RANK(uint64_t);
uint64_t BANKGROUP(uint64_t);
uint64_t BANK(uint64_t);
uint64_t ROW(uint64_t);
uint64_t COLUMN(uint64_t);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

constexpr size_t B_CH = Log2<NUM_CHANNELS>::value;
constexpr size_t B_SC = Log2<NUM_SUBCHANNELS>::value;
constexpr size_t B_RA = Log2<NUM_RANKS>::value
constexpr size_t B_BG = Log2<NUM_BANKGROUPS>::value;
constexpr size_t B_BA = Log2<NUM_BANKS>::value;
constexpr size_t B_RO = Log2<NUM_ROWS>::value;
constexpr size_t B_CO = Log2<NUM_COLUMNS>::value;

inline consteval uint64_t MASK(size_t B) {
    return (1L << B) - 1;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#if DRAM_ADDRESS_MAPPING="MOP4"
#include "address/mop4.inl"
#else
#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // DRAM_ADDRESS_h
