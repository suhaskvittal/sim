/* 
 *  author: Suhas Vittal
 *  date:   10 October 2024
 * */

#include "utils/bitcount.h"
#include "os.h"

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

__TEMPLATE_HEADER__ bool
__TEMPLATE_CLASS__::probe(uint64_t lineaddr) {
    ++s_accesses_;

    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    // Check access.
    auto it = s.find(t);
    if (it != s.end()) {
        // On hit: update metadata
        if constexpr (POL == CacheReplPolicy::LRU) {
#ifdef IMPLEMENT_LRU_WITH_FIFO
            auto _it = std::find(s.lru_fifo_.begin(), s.lru_fifo_.end(), t);
            s.lru_fifo_.erase(_it);
            s.lru_fifo_.push_back(t);
#else
            it->second.lru_timestamp_ = GL_cycle_;
#endif
        } else if constexpr (POL == CacheReplPolicy::SRRIP || POL == CacheReplPolicy::BRRIP) {
            it->second.rrpv_ = SRRIP_MAX;
        }
        return true;
    } else {
        ++s_misses_;
        return false;
    }
}

__TEMPLATE_HEADER__ bool
__TEMPLATE_CLASS__::fill(uint64_t lineaddr, size_t num_mshr_refs, uint64_t& vic) {
    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    bool is_wb = false;
    if (s.size() >= W) {
        auto it = select_victim(s);
        vic = join_lineaddr(it->first, k);
        is_wb = it->second.dirty_;
        s.erase(it);
    }
    // Create cache entry.
    uint8_t rrpv;
    if constexpr (POL == CacheReplPolicy::SRRIP || POL == CacheReplPolicy::BRRIP) {
        if (num_mshr_refs > 1) {
            rrpv = SRRIP_MAX;
        } else {
            if constexpr (POL == CacheReplPolicy::BRRIP) {
                rrpv = (GL_RNG_() & 7) ? 1 : 0;
            } else {
                rrpv = 1;
            }
        }
    }
    s[t] = (CacheEntry) { false, GL_cycle_, rrpv };
    return is_wb;
}

__TEMPLATE_HEADER__ bool
__TEMPLATE_CLASS__::mark_dirty(uint64_t lineaddr) {
    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    auto it = s.find(t);
    if (it != s.end()) {
        it->second.dirty_ = true;
        return true;
    } else {
        return false;
    }
}

__TEMPLATE_HEADER__ inline void
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
        return s.lru();
    } else if constexpr (POL == CacheReplPolicy::RAND) {
        return s.rand();
    } else if constexpr (POL == CacheReplPolicy::SRRIP || POL == CacheReplPolicy::BRRIP) {
        return s.srrip();
    } else {
        std::cerr << "Unsupported cache replacement policy.\n";
        exit(1);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::print_stats(std::ostream& out, std::string_view cache_name) {
    double miss_rate = ((double)s_misses_) / ((double)s_accesses_);

    PRINT_STAT(out, cache_name, "MISSES", s_misses_);
    PRINT_STAT(out, cache_name, "ACCESSES", s_accesses_);
    PRINT_STAT(out, cache_name, "MISS_RATE", 100*miss_rate);

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
