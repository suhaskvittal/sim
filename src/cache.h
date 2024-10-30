/*
 * author:  Suhas Vittal
 * date:    10 October 2024 
 * */

#ifndef CACHE_h
#define CACHE_h

#include "defs.h"

#include <array>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <string_view>

#include <zlib.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct CacheEntry {
    bool dirty_ =false;
    uint64_t lru_timestamp_;
    uint8_t rrpv_;
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct CacheSet : public std::unordered_map<uint64_t, CacheEntry> {
    /*
     * For use with Belady's min (OPT) replacement. Each key of the map
     * is a physical address, and the values are a queue of instno.
     * */
    std::unordered_map<uint64_t, std::deque<uint64_t>> belady_access_stream_;
    /*
     * Cache Replacement Policies:
     *  Defined in `cache/replacement.cpp`
     * */
    CacheSet::iterator lru(void);
    CacheSet::iterator rand(void);
    CacheSet::iterator srrip(void);
    CacheSet::iterator belady(void);
    CacheSet::iterator prowb(void);

    inline bool 
    any_belady_stream_empty() {
        for (auto it = belady_access_stream_.begin(); it != belady_access_stream_.end(); it++) {
            if (it->second.empty()) return true;
        }
        return false;
    }
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
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

    gzFile belady_trace_in_[N_THREADS];
public:
    Cache(void);
    ~Cache(void);
    /*
     * Basic cache functions:
     *  `probe`: returns true if the given line is in the cache.
     *  `fill`: installs the given line into the cache. Returns true if `victim` needs to be written back.
     *  `invalidate`: removes the given line if it exists.
     *  `mark_dirty`: sets the dirty bit for the given line
     * */
    bool probe(uint64_t);
    bool fill(uint64_t, size_t num_mshr_refs, uint64_t& victim);
    void invalidate(uint64_t);
    bool mark_dirty(uint64_t);

    void print_stats(std::ostream&, std::string_view cache_name);

    void set_belady_trace_file(std::string trace_file, size_t coreid);
    void populate_sets_with_belady(size_t coreid, uint64_t inst_limit);
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

    void repopulate_set_with_belady_until_all_streams_nonempty(CacheSet&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#include "cache.tpp"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // CACHE_h
