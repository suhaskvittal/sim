/*
 *  author: Suhas Vittal
 *  date:   10 October 2024
 * */

#include <string.h>
#include <strings.h>

#define __TEMPLATE_HEADER__ template <class CACHE_TYPE>
#define __TEMPLATE_CLASS__ CacheController<CACHE_TYPE>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__
__TEMPLATE_CLASS__::CacheController(uint64_t cache_lat)
    :cache_latency_(cache_lat)
{}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::tick() {
    for (auto it = bounced_requests_.begin(); it != bounced_requests_.end(); )
    {
        const auto& [lineaddr, is_load, mshr_id] = *it;
        if (ds3i_->make_request(lineaddr, is_load, mshr_id)) {
            it = bounced_requests_.erase(it);
        } else {
            ++it;
        }
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ int
__TEMPLATE_CLASS__::access(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load) {
    int retval;
    // NOTE: does not handle case where `lineaddr` is currently in the MSHR and we want to do a load. This does
    // not matter in simulation as this new access can only finish when the existing access finishes. So, we
    // can just treat this as a cache hit.
    uint64_t vic;
    if (is_load) {
        if (!avail_mshr_ids_) {
            return -1;
        }
    }
    CacheResult r = cache_.access(lineaddr, is_load, vic);
    retval = (r==CacheResult::HIT) ? 1 : 0;
    if (is_load) {
        if (r == CacheResult::HIT) {
            GL_cores_[coreid].rob_[robid].end_cycle_ = GL_cycle_ + cache_latency_;
            retval = 1;
        } else {
            add_mshr_entry(lineaddr, coreid, robid, inst_num);
            retval = 0;
        }
    }
    // Handle writeback to DRAM.
    if (r == CacheResult::MISS_WITH_WB) {
        if (!ds3i_->make_request(vic, false, 0)) {
            bounced_requests_.emplace_back(vic, false, 0);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ inline void
__TEMPLATE_CLASS__::mark_as_finished(size_t i) {
    MSHREntry& e = mshr_[i];
    GL_cores_[e.coreid_].rob_[ e.robid_ ].end_cycle_ = GL_cycle_ + cache_latency_;
    // release MSHR entry
    avail_mshr_ids_ |= (1L << i);
    // update stats.
    ++s_num_delays_;
    s_tot_delay_ += GL_cycle_ - e.cycle_fired_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::print_stats(std::ostream& out, std::string cache_name) {
    cache_.print_stats(out, cache_name);

    double miss_penalty = ((double)s_tot_delay_) / ((double)s_num_delays_);
    PRINT_STAT(out, cache_name + "_MISS_PENALTY", miss_penalty);
    out << "\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::dump_debug_info(std::ostream& out) {
    out << "CACHE CONTROLLER DUMP ----------------------------------\n";
    out << "\tMSHR:\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::add_mshr_entry(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num) {
    size_t i = ffsll(avail_mshr_ids_)-1;

    mshr_[i].coreid_ = coreid;
    mshr_[i].robid_ = robid;
    mshr_[i].inst_num_ = inst_num;
    mshr_[i].cycle_fired_ = GL_cycle_;

    // Send command to DRAM.
    if (!ds3i_->make_request(lineaddr, true, i)) {
        bounced_requests_.emplace_back(lineaddr, true, i);
    }
    avail_mshr_ids_ &= ~(1L << i);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#undef __TEMPLATE_HEADER__
#undef __TEMPLATE_CLASS__

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
