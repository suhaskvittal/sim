/*
 *  author: Suhas Vittal
 *  date:   28 October 2024
 *  */

#ifndef CACHE_CONTROLLER_LLC2_h
#define CACHE_CONTROLLER_LLC2_h

#include "cache/controller.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

using LLC=Cache<LLC_SIZE_KB, LLC_ASSOC, LLC_REPL_POLICY>;

class LLC2Controller : public CacheController<LLC2Controller, LLC> {
public:
    constexpr static size_t             MSHR_SIZE = 512;
    constexpr static uint64_t           CACHE_LATENCY = 24;
    constexpr static std::string_view   CACHE_NAME = "LLC";
    constexpr static CacheHitPolicy     CACHE_HIT_POLICY = CacheHitPolicy::DEFAULT;
public:
    void update_prev_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t latency); 
    int access_next_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load); 
    void _tick(void);
    void _print_stats(std::ostream&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // CACHE_CONTROLLER_LLC2_h
