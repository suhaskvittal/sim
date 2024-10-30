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
enum class CacheReplPolicy  { LRU, RAND, SRRIP, BRRIP, BELADY };
enum class CacheHitPolicy   { DEFAULT, INVALIDATE };

inline std::string_view
repl_policy_name(CacheReplPolicy p) {
    if (p == CacheReplPolicy::LRU)      return "Least Recently Used";
    if (p == CacheReplPolicy::RAND)     return "Random";
    if (p == CacheReplPolicy::SRRIP)    return "Static Re-Reference Interval Prediction";
    if (p == CacheReplPolicy::BRRIP)    return "Bimodal Re-Reference Interval Prediction";
    if (p == CacheReplPolicy::BELADY)   return "Belady's Min (OPT)";
    return "Unknown Cache Policy";
}

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
 * Other constants.
 * */
constexpr size_t ROB_WIDTH = 256;
constexpr size_t OS_PAGESIZE = 4096;
constexpr size_t LINESIZE = 64;
constexpr size_t LINES_PER_PAGE = OS_PAGESIZE / LINESIZE;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class OS;
class Core;
class LLC2Controller;
class DS3Interface;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

extern uint64_t         GL_cycle_;

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

#define INCREMENT_AND_MOD(x,mod)            ((++(x))==(mod)) ? 0 : (x)
#define INCREMENT_AND_MOD_BY_POW2(x,mod)    (((x)+1) & ((mod)-1))

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif
