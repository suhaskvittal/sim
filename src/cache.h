/*
 * author:  Suhas Vittal
 * date:    10 October 2024 
 * */

#ifndef CACHE_h
#define CACHE_h

#include "defs.h"

#include <array>
#include <iostream>
#include <unordered_map>

/*
 * Basic cache data structures.
 * */
struct CacheEntry {
    bool dirty_ =false;
    uint64_t lru_timestamp_;
};

using CacheSet = std::unordered_map<uint64_t, CacheEntry>;

/*
 * General class for caches. Replacement policy
 * is LRU.
 * */
template <size_t SIZE_KB, size_t WAYS, CacheReplPolicy REPL_POLICY=CacheReplPolicy::LRU>
class Cache {
public:
    constexpr static size_t SETS = (SIZE_KB*1024)/(WAYS*LINESIZE);

    uint64_t s_misses_ =0;
    uint64_t s_accesses_ =0;
private:
    std::array<CacheSet, SETS> sets_;
public:
    Cache(void);
    /*
     * Accesses the cache and installs the given `lineaddr` if it is not currently
     * in the cache. If a line is evicted, then `victim_lineaddr` is populated.
     *
     * See `CacheResult` for the possible outcomes.
     * */
    CacheResult access(uint64_t lineaddr, bool is_write, uint64_t& victim_lineaddr);

    void invalidate(uint64_t);
    void mark_dirty(uint64_t);

    void print_stats(std::ostream&, std::string cache_name);
private:
    /*
     * Searches for a victim within a set. This function assumes that
     * all lines in the set are valid. Each replacement policy is implemented
     * as a `if constexpr` statement. Specific implementations of replacement policies
     * are separate functions (see `cache/replacement.h`).
     *
     * Returns the potential victim.
     * */
    CacheSet::iterator select_victim(CacheSet&);

    void     split_lineaddr(uint64_t, uint64_t& tag, uint64_t& set);
    uint64_t join_lineaddr(uint64_t tag, uint64_t set);
};
/*
 * Cache Replacement Policies:
 *  All functions here must have the signature: CacheSet::iterator(CacheSet&).
 *
 *  Defined in `cache/replacement.cpp`
 * */
CacheSet::iterator lru(CacheSet&);
CacheSet::iterator rand(CacheSet&);
CacheSet::iterator ssrip(CacheSet&);

#include "cache.tpp"

#endif  // CACHE_h
