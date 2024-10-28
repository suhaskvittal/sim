/* author: Suhas Vittal
 *  date:   10 October 2024
 * */

#include "utils/bitcount.h"

#include <iomanip>

#define __TEMPLATE_HEADER__ template <size_t C, size_t W, CacheReplPolicy POL>
#define __TEMPLATE_CLASS__  Cache<C,W,POL>      

constexpr uint8_t SRRIP_MAX = static_cast<uint8_t>( (1<<SRRIP_WIDTH)-1 );

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__
__TEMPLATE_CLASS__::Cache() {
    // Each cache set will have `W` (number of ways) entries.
    for (CacheSet& s : sets_) {
        s.reserve(W);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ CacheResult
__TEMPLATE_CLASS__::access(uint64_t lineaddr, bool is_write, uint64_t& victim_lineaddr) {
    if (is_write) ++s_writes_;
    else          ++s_reads_;

    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    // Check if the line is in the cache.
    auto it = s.find(t);
    if (it != s.end()) {
        // Tag hit: update metadata.
        it->second.dirty_ |= is_write;
        it->second.lru_timestamp_ = GL_cycle_;
        it->second.rrpv_ = SRRIP_MAX;
        return CacheResult::HIT;
    } else {
        if (is_write) ++s_write_misses_;
        else          ++s_read_misses_;
        // Get victim entry from set.
        CacheResult miss_type = CacheResult::MISS_NO_WB;
        if (s.size() == W) {
            // Need to select victim from cache.
            it = select_victim(s);
            victim_lineaddr = it->first;
            if (it->second.dirty_) {
                miss_type = CacheResult::MISS_WITH_WB;
            }
            s.erase(it);
        }
        // Install new line:
        //  For computing re-reference predictor value (`rrpv`): value depends on whether
        //  we are using Bimodel RRIP or Static RRIP
        uint8_t rrpv;
        if (is_write) {
            rrpv = SRRIP_MAX;  // We can only have a write miss if it is the beginning of the trace,
                               // where lines have not been installed. So, treat this as a hit for
                               // the purposes of RRIP.
        } else {
            if constexpr (POL == CacheReplPolicy::BRRIP) {
                if (rand() & 7 == 0) rrpv = 1;
                else                 rrpv = 0;
            } else {
                rrpv = 1;
            }
        }

        s[t] = (CacheEntry) { 
            is_write,
            GL_cycle_,
            rrpv
        };
        return miss_type;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ inline bool
__TEMPLATE_CLASS__::probe(uint64_t lineaddr) {
    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);
    return sets_.at(k).count(t);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::mark_dirty(uint64_t lineaddr) {
    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    auto it = s.find(t);
    if (it != s.end()) {
        it->second.dirty_ = true;
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::invalidate(uint64_t lineaddr) {
    uint64_t t, s;
    split_lineaddr(lineaddr, t, s);
    sets_.at(s).erase(t);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ CacheSet::iterator
__TEMPLATE_CLASS__::select_victim(CacheSet& s) {
    if constexpr (POL == CacheReplPolicy::LRU) {
        return lru(s);
    } else if constexpr (POL == CacheReplPolicy::RAND) {
        return rand(s);
    } else if constexpr (POL == CacheReplPolicy::SRRIP || POL == CacheReplPolicy::BRRIP) {
        return srrip(s);
    } else {
        std::cerr << "Unsupported cache replacement policy.\n";
        exit(1);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::print_stats(std::ostream& out, std::string_view cache_name) {
    double rmr = ((double)s_read_misses_) / ((double)s_reads_);
    double wmr = ((double)s_write_misses_) / ((double)s_writes_);

    PRINT_STAT(out, cache_name, "READ_MISSES", s_read_misses_);
    PRINT_STAT(out, cache_name, "READS", s_reads_);
    PRINT_STAT(out, cache_name, "READ_MISS_RATE", 100*rmr);

    PRINT_STAT(out, cache_name, "WRITE_MISSES", s_write_misses_);
    PRINT_STAT(out, cache_name, "WRITES", s_writes_);
    PRINT_STAT(out, cache_name, "WRITE_MISS_RATE", 100*wmr);

    out << "\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ inline void
__TEMPLATE_CLASS__::split_lineaddr(uint64_t lineaddr, uint64_t& t, uint64_t& s) {
    s = lineaddr & (SETS-1);
    t = lineaddr >> Log2<SETS>::value;
}

__TEMPLATE_HEADER__ inline uint64_t
__TEMPLATE_CLASS__::join_lineaddr(uint64_t t, uint64_t s) {
    return (t << Log2<SETS>::value) | s;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#undef __TEMPLATE_HEADER__
#undef __TEMPLATE_CLASS__

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
