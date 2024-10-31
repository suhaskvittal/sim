#ifndef CONSTANTS_h
#define CONSTANTS_h

#include <iostream>
#include <iomanip>
#include <string_view>
#include <random>

#include <stdint.h>
#include <stddef.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#ifndef N_THREADS
#define N_THREADS 1
#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * Enum declarations.
 * */
enum class CacheResult      { HIT, MISS_NO_WB, MISS_WITH_WB };
enum class CacheReplPolicy  { LRU, RAND, SRRIP, BRRIP };
enum class CacheHitPolicy   { DEFAULT, INVALIDATE };

inline std::string_view
repl_policy_name(CacheReplPolicy p) {
    if (p == CacheReplPolicy::LRU)      return "Least Recently Used";
    if (p == CacheReplPolicy::RAND)     return "Random";
    if (p == CacheReplPolicy::SRRIP)    return "Static Re-Reference Interval Prediction";
    if (p == CacheReplPolicy::BRRIP)    return "Bimodal Re-Reference Interval Prediction";
    return "Unknown Cache Policy";
}

enum class DRAMPagePolicy { OPEN, CLOSED };
enum class DRAMRefreshMethod { REFAB, REFSB };
enum class DRAMCommandType {
    READ, 
    WRITE,
    ACTIVATE,
    PRECHARGE,
    READ_PRECHARGE,
    WRITE_PRECHARGE,
    REFRESH
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * Cache definitions.
 * */
constexpr size_t L2C_SIZE_KB = 1024;
constexpr size_t L2C_ASSOC = 16;

#ifndef L2_REPL_POLICY
#define L2_REPL_POLICY CacheReplPolicy::LRU
#endif

constexpr size_t LLC_SIZE_KB_PER_CORE = 2*1024;
constexpr size_t LLC_SIZE_KB = LLC_SIZE_KB_PER_CORE * N_THREADS;
constexpr size_t LLC_ASSOC = 8;

#ifndef LLC_REPL_POLICY
#define LLC_REPL_POLICY CacheReplPolicy::LRU
#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

constexpr size_t SRRIP_WIDTH = 3;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * DRAM definitions.
 * */

constexpr DRAMPagePolicy    DRAM_PAGE_POLICY = DRAMPagePolicy::OPEN;
constexpr DRAMRefreshMethod DRAM_REFRESH = DRAMRefreshMethod::REFAB;
/*
 * Note on below constants: -- they are for an x4 32GB single channel system.
 *  `NUM_SUBCHANNELS` is per channel
 *  `NUM_BANKGROUPS` is per rank
 *  `NUM_BANKS` is per bankgroup.
 *  (etc.)
 *
 *  `COLUMN_WIDTH` is also the bus width for a subchannel.
 *
 *  `RANK_SIZE_MB` and `NUM_RANKS` are determined by the other parameters.
 * */
constexpr size_t NUM_CHANNELS = 1;
constexpr size_t NUM_SUBCHANNELS = 2;
constexpr size_t NUM_BANKGROUPS = 8;
constexpr size_t NUM_BANKS = 4;
constexpr size_t NUM_ROWS = 1L << 16;
constexpr size_t NUM_COLUMNS = 2048;

constexpr size_t COLUMN_WIDTH = 32;

constexpr size_t CHANNEL_SIZE_MB = 1L << 15;
constexpr size_t SUBCHANNEL_SIZE_MB = CHANNEL_SIZE_MB / NUM_SUBCHANNELS;
constexpr size_t BANK_SIZE_MB = NUM_ROWS * (NUM_COLUMNS * COLUMN_WIDTH)/8;

constexpr size_t RANK_SIZE_MB = SUBCHANNEL_SIZE_MB / (BANK_SIZE_MB * NUM_BANKS * NUM_BANKGROUPS);
constexpr size_t NUM_RANKS = SUBCHANNEL_SIZE_MB/RANK_SIZE_MB;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * Other constants.
 * */
constexpr size_t ROB_WIDTH = 256;
constexpr size_t OS_PAGESIZE = 4096;
constexpr size_t LINESIZE = 64;
constexpr size_t LINES_PER_PAGE = OS_PAGESIZE / LINESIZE;

constexpr size_t BURST_LENGTH = LINESIZE / COLUMN_WIDTH;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class OS;
class Core;
class LLC2Controller;
class DS3Interface;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

extern uint64_t         GL_cycle_;
extern uint64_t         GL_dram_cycle_;

extern OS*              GL_os_;
extern Core*            GL_cores_[N_THREADS];
extern LLC2Controller*  GL_llc_controller_;
extern DS3Interface*    GL_memory_controller_;

extern std::mt19937_64  GL_RNG_;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline void 
TAG_VA_WITH_COREID(uint64_t& va, size_t coreid) {
    va |= static_cast<uint64_t>(coreid) << 48;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

template <class NUMBER> inline void
PRINT_STAT(std::ostream& out, std::string name, NUMBER value) {
    out << std::setw(24) << std::left << name << "\t" << value << "\n";
}

template <class NUMBER> inline void
PRINT_STAT(std::ostream& out, std::string_view header, std::string name, NUMBER value) {
    return PRINT_STAT(out, std::string(header) + "_" + name, value);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline
std::string FMT_BIGNUM(uint64_t num) {
    std::string suffix;
    if (num < 1'000'000'000) {
        num /= 1'000'000;
        suffix = "M";
    } else {
        num /= 1'000'000'000;
        suffix = "B";
    }
    return std::to_string(num) + suffix;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#define MOD_BY_POW2(x,mod)  ((x) & ((mod)-1))

#define INCREMENT_AND_MOD(x,mod)            ((++(x))==(mod)) ? 0 : (x)
#define INCREMENT_AND_MOD_BY_POW2(x,mod)    (((x)+1) & ((mod)-1))

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif
