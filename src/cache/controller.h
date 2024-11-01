/*
 * author: Suhas Vittal
 *  date:   26 October 2024
 * */

#ifndef CACHE_CONTROLLER_h
#define CACHE_CONTROLLER_h

#include "defs.h"
#include "cache.h"

#include <string_view>
#include <unordered_map>
#include <vector>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct MSHREntry {
    uint8_t  coreid_;
    uint16_t robid_;
    uint64_t inst_num_;
    /*
     * Stats for miss penalty computation.
     * */
    uint64_t cycle_fired_;

    MSHREntry(void) =default;
    MSHREntry(const MSHREntry&) =default;
    MSHREntry(size_t coreid, size_t robid, uint64_t inst_num)
        :coreid_( static_cast<uint8_t>(coreid) ),
        robid_( static_cast<uint16_t>(robid) ),
        inst_num_(inst_num),
        cycle_fired_(GL_cycle_)
    {}
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/*
 * Generic cache controller class. The intended use of this class
 * is via static polymorphism, where the class `IMPL` should
 * inherit from `CacheController<IMPL>`.
 *
 * `IMPL` should define the following:
 *      Constexpr:
 *          size_t              `MSHR_SIZE`
 *          uint64_t            `CACHE_LATENCY`
 *          std::string_view    `CACHE_NAME`
 *          CacheHitPolicy      `CACHE_HIT_POLICY`
 *      Functions:
 *          `void update_prev_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t latency);
 *              --> Tells previous level in the hierarchy that an access has completed (i.e. LLC notifies L2).
 *
 *          `int access_next_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load)`
 *              --> Requests line from next level in the hierarchy (i.e. L2 asks LLC, or LLC asks memory).
 *              --> Same signature and return values as `access` (see below).
 *
 *          `void _tick(void)`: this is just a function that is called by
 *              `tick` below in case `IMPL` wants to implement anything extra.
 *          `void _print_stats(std::ostream&)`: this prints extra stats
 *              for `IMPL`: called by `print_stats`
 *
 * This class manages statistics and accesses.
 * */
template <class IMPL, class CACHE_TYPE>
class CacheController {
public:
    CACHE_TYPE cache_;

    uint64_t s_num_delays_=0;
    uint64_t s_tot_delay_ =0;
    uint64_t s_mshr_full_ =0;

    uint64_t s_tot_write_use_dist_ =0;
    uint64_t s_num_write_use_ =0;
    uint64_t s_num_write_use_hits_ =0;
protected:
    std::unordered_multimap<uint64_t, MSHREntry> mshr_;
    std::vector<std::tuple<uint64_t,bool>> bounced_requests_;

    std::unordered_map<uint64_t, uint64_t> write_use_cycle_map_;
    uint64_t access_ctr_ =0;
public:
    CacheController(void);

    void tick(void);
    /*
     * Performs the given access to the `cache` and handles state updates.
     * Returns -1 if the MSHR is full, 0 if the access occurred and was a cache miss, and 1 if it was a hit.
     * */
    int access(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load);
    /*
     * Marks the corresponding entry in the MSHR as finished. Calls
     *  `IMPL::
     * */
    void mark_as_finished(uint64_t lineaddr);

    void print_stats(std::ostream&);
private:
    /*
     * Creates MSHR entry. If the address is fresh in the MSHR, an access is also
     * made.
     * */
    void add_mshr_entry(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#include "controller.tpp"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // CACHE_CONTROLLER_h
