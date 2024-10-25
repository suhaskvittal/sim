/*
 *  author: Suhas Vittal
 *  date:   10 October 2024
 * */

#include "utils/bitcount.h"

#include <iomanip>

#define __TEMPLATE_HEADER__ template <size_t C, size_t W, CacheReplPolicy POL>
#define __TEMPLATE_CLASS__  Cache<C,W,POL>      

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
    ++s_accesses_;

    uint64_t t, k;
    split_lineaddr(lineaddr, t, k);

    CacheSet& s = sets_.at(k);
    // Check if the line is in the cache.
    auto it = s.find(t);
    if (it != s.end()) {
        // Tag hit: update metadata.
        it->second.dirty_ |= is_write;
        it->second.lru_timestamp_ = GL_cycle_;
        return CacheResult::HIT;
    } else {
        ++s_misses_;
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
        s[t] = (CacheEntry) { is_write, GL_cycle_ };
        return miss_type;
    }
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
    } else {
        std::cerr << "Unsupported cache replacement policy.\n";
        exit(1);
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

__TEMPLATE_HEADER__ void
__TEMPLATE_CLASS__::print_stats(std::ostream& out, std::string cache_name) {
    double miss_rate = ((double)s_misses_) / ((double)s_accesses_);

    PRINT_STAT(out, cache_name + "_MISSES", s_misses_);
    PRINT_STAT(out, cache_name + "_ACCESSES", s_accesses_);
    PRINT_STAT(out, cache_name + "_MISS_RATE", 100*miss_rate);
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
