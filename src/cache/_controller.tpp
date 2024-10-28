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
{
    mshr_.reserve(MSHR_SIZE);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::tick() {
    for (auto it = bounced_requests_.begin(); it != bounced_requests_.end(); )
    {
        const auto& [lineaddr, is_load] = *it;
        if (ds3i_->make_request(lineaddr, is_load)) {
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
        if (mshr_.size() == MSHR_SIZE) {
            return -1;
        }
    }
    CacheResult r = cache_.access(lineaddr, !is_load, vic);
    retval = (r==CacheResult::HIT) ? 1 : 0;
    if (is_load) {
        if (write_use_cycle_map_.count(lineaddr)) {
            ++s_num_write_use_;
            s_tot_write_use_dist_ += access_ctr_ - write_use_cycle_map_[lineaddr];
            if (r == CacheResult::HIT) ++s_num_write_use_hits_;

            write_use_cycle_map_.erase(lineaddr);
        }
        // If `lineaddr` is in the MSHR but we had a miss, then this means that we are
        // still waiting for the data to come back from memory. Just give this a cache
        // hit latency.
        if (r == CacheResult::HIT || mshr_.count(lineaddr)) {
            GL_cores_[coreid].rob_[robid].end_cycle_ = GL_cycle_ + cache_latency_;
        } else {
            add_mshr_entry(lineaddr, coreid, robid, inst_num);
        }
    } else {
        write_use_cycle_map_[lineaddr] = access_ctr_;
    }
    ++access_ctr_;
    // Handle writeback to DRAM.
    if (r == CacheResult::MISS_WITH_WB) {
        if (!ds3i_->make_request(vic, false)) {
            bounced_requests_.emplace_back(vic, false);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ inline void
__TEMPLATE_CLASS__::mark_as_finished(uint64_t lineaddr) {
    MSHREntry& e = mshr_.at(lineaddr);
    GL_cores_[e.coreid_].rob_[ e.robid_ ].end_cycle_ = GL_cycle_ + cache_latency_;
    // release MSHR entry
    mshr_.erase(lineaddr);
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

    double mean_write_use_dist = ((double)s_tot_write_use_dist_)/((double)s_num_write_use_);
    
    PRINT_STAT(out, cache_name + "_WRITE_USES", s_num_write_use_);
    PRINT_STAT(out, cache_name + "_WRITE_USE_HITS", s_num_write_use_hits_);
    PRINT_STAT(out, cache_name + "_MEAN_WRITE_USE_DIST", mean_write_use_dist);

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
    MSHREntry e = { static_cast<uint8_t>(coreid), static_cast<uint16_t>(robid), inst_num, GL_cycle_ };
    mshr_[lineaddr] = e;
    // Send command to DRAM.
    if (!ds3i_->make_request(lineaddr, true)) {
        bounced_requests_.emplace_back(lineaddr, true);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#undef __TEMPLATE_HEADER__
#undef __TEMPLATE_CLASS__

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
