/*
 *  author: Suhas Vittal
 *  date:   26 October 2024
 * */

#include <strings.h>

#define __TEMPLATE_HEADER__ template <class IMPL, class CACHE_TYPE>
#define __TEMPLATE_CLASS__ CacheController<IMPL, CACHE_TYPE>

#define __CALL_CHILD__(func)    static_cast<IMPL*>(this)->func

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__
__TEMPLATE_CLASS__::CacheController() {
    mshr_.reserve(IMPL::MSHR_SIZE);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::tick() {
    for (auto it = bounced_requests_.begin(); it != bounced_requests_.end(); )
    {
        const auto& [lineaddr, is_load] = *it;
        int retval;
        if (is_load) {
            MSHREntry& e = mshr_.at(lineaddr);
            retval = __CALL_CHILD__(access_next_level(lineaddr, e.coreid_, e.robid_, e.inst_num_, true) );
        } else {
            retval = __CALL_CHILD__(access_next_level(lineaddr, 0, 0, 0, false));
        }
        if (retval == -1) it = bounced_requests_.erase(it);
        else              ++it;
    }
    __CALL_CHILD__( _tick() );
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ int
__TEMPLATE_CLASS__::access(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load) {
    // Early exit condition: must have space in `mshr_`.
    if (is_load && mshr_.size() == IMPL::MSHR_SIZE) return -1;
    // NOTE: does not handle case where `lineaddr` is currently in the MSHR and we want to do a load. This does
    // not matter in simulation as this new access can only finish when the existing access finishes. So, we
    // can just treat this as a cache hit.

    uint64_t vic;
    CacheResult r = cache_.access(lineaddr, !is_load, vic);
    if (r == CacheResult::HIT) {
        if constexpr (IMPL::CACHE_HIT_POLICY == CacheHitPolicy::INVALIDATE) {
            cache_.invalidate(lineaddr);
        }
        // Perform updates if necessary.
        if (is_load) {
            __CALL_CHILD__(update_prev_level(lineaddr, coreid, robid, GL_cycle_ + IMPL::CACHE_LATENCY));
        } else {
            write_use_cycle_map_[lineaddr] = access_ctr_;
        }
    } else {
        if (is_load) add_mshr_entry(lineaddr, coreid, robid, inst_num);

        if ( r == CacheResult::MISS_WITH_WB
            && __CALL_CHILD__(access_next_level(vic, 0, 0, 0, false)) == -1 )
        {
            bounced_requests_.emplace_back(vic, false);
        }
    }
    ++access_ctr_;
    return (r==CacheResult::HIT) ? 1 : 0;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::mark_as_finished(uint64_t lineaddr) {
    auto it = mshr_.find(lineaddr);
    MSHREntry& e = it->second;
    __CALL_CHILD__(
            update_prev_level(lineaddr, e.coreid_, e.robid_, GL_cycle_ - e.cycle_fired_ + IMPL::CACHE_LATENCY));
    mshr_.erase(it);

    ++s_num_delays_;
    s_tot_delay_ += GL_cycle_ - e.cycle_fired_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::print_stats(std::ostream& out) {
    cache_.print_stats(out, IMPL::CACHE_NAME);

    double miss_penalty = ((double)s_tot_delay_) / ((double)s_num_delays_);
    PRINT_STAT(out, IMPL::CACHE_NAME, "MISS_PENALTY", miss_penalty);

    double mean_write_use_dist = ((double)s_tot_write_use_dist_)/((double)s_num_write_use_);
    PRINT_STAT(out, IMPL::CACHE_NAME, "WRITE_USES", s_num_write_use_);
    PRINT_STAT(out, IMPL::CACHE_NAME, "WRITE_USE_HITS", s_num_write_use_hits_);
    PRINT_STAT(out, IMPL::CACHE_NAME, "MEAN_WRITE_USE_DIST", mean_write_use_dist);

    out << "\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::add_mshr_entry(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num) {
    MSHREntry e(coreid, robid, inst_num);
    mshr_[lineaddr] = e;
    // Send command to DRAM.
    if (__CALL_CHILD__(access_next_level(lineaddr, coreid, robid, inst_num, true)) == -1) {
        bounced_requests_.emplace_back(lineaddr, true);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#undef __TEMPLATE_HEADER__
#undef __TEMPLATE_CLASS__
#undef __CALL_CHILD__

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
