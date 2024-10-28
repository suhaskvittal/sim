/*
 *  author: Suhas Vittal
 *  date:   26 October 2024
 * */

#ifndef CACHE_CONTROLLER_LLC_h
#define CACHE_CONTROLLER_LLC_h

#include "cache/controller.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class LLCController : public CacheController<LLCController> {
public:
    using CACHE_TYPE=Cache<LLC_SIZE_KB, LLC_ASSOC, LLC_REPL_POLICY>;

    constexpr static size_t             MSHR_SIZE = 32*N_THREADS;
    constexpr static uint64_t           CACHE_LATENCY = 16;
    constexpr static std::string_view   CACHE_NAME = "LLC";
    constexpr static CacheHitPolicy     CACHE_HIT_POLICY = CacheHitPolicy::INVALIDATE;
private:
    void update_prev_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t latency); 
    int access_next_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load); 
    void _tick(void);
    void _print_stats(std::ostream&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // CACHE_CONTROLLER_LLC_h
