/*
 *  author: Suhas Vittal
 *  date:   10 October 2024
 * */

#ifndef CACHE_CONTROLLER_h
#define CACHE_CONTROLLER_h

#include "defs.h"

#include "cache.h"
#include "core.h"
#include "ds3/interface.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

struct MSHREntry {
    uint8_t  coreid_;
    uint16_t robid_;
    uint64_t inst_num_;
    /*
     * Stats for miss penalty computation.
     * */
    uint64_t cycle_fired_;
};

template <class CACHE_TYPE>
class CacheController {
public:
    constexpr static size_t MSHR_SIZE = 512;

    CACHE_TYPE cache_;
    DS3Interface* ds3i_;

    uint64_t cycle_ =0;

    uint64_t s_num_delays_=0;
    uint64_t s_tot_delay_ =0;
    uint64_t s_mshr_full_ =0;

    const uint64_t cache_latency_;
private:
    std::unordered_map<uint64_t, MSHREntry> mshr_;
    /*
     * Cache controller needs to retry failed reads/writes. Each entry is of the form
     * "lineaddr,is_load".
     * */
    std::vector< std::tuple<uint64_t, bool> > bounced_requests_;
public:
    CacheController(uint64_t cache_latency);

    void tick(void);
    /*
     * Performs the given access to the `cache` and handles state updates.
     *
     * Returns -1 if the MSHR is full, 0 if the access occurred and was a cache miss, and 1 if it was a hit.
     * */
    int access(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load);
    /*
     * Marks the corresponding entry in the MSHR as finished.
     * */
    void mark_as_finished(uint64_t lineaddr);

    void print_stats(std::ostream&, std::string cache_name);
    void dump_debug_info(std::ostream&);
private:
    /*
     * Creates MSHR entry. If the address is fresh in the MSHR, an access is also
     * made.
     * */
    void add_mshr_entry(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num);
};

#include "controller.tpp"

#endif  // CACHE_CONTROLLER_h
