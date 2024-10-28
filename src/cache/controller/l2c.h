/*
 *  author: Suhas Vittal
 *  date:   26 October 2024
 * */

#ifndef CACHE_CONTROLLER_L2C_h
#define CACHE_CONTROLLER_L2C_h

#include "cache/controller.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class L2Controller : public CacheController<L2Controller> {
public:
    using CACHE_TYPE=Cache<L2_SIZE_KB, L2_ASSOC, L2_REPL_POLICY>;

    constexpr static size_t             MSHR_SIZE = 32;
    constexpr static uint64_t           CACHE_LATENCY = 8;
    constexpr static std::string_view   CACHE_NAME = "L2";
    constexpr static CacheHitPolicy     CACHE_HIT_POLICY = CacheHitPolicy::DEFAULT;
private:
    void update_prev_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t latency); 
    int access_next_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load); 
    void _tick(void);
    void _print_stats(std::ostream&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // CACHE_CONTROLLER_L2C_h
