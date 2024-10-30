/*
 *  author: Suhas Vittal
 *  date:   24 October 2024
 * */

#include "cache.h"

#include <algorithm>
#include <limits>

#include <stdlib.h>

CacheSet::iterator
CacheSet::lru() {
    auto v_it = begin();
    for (auto it = std::next(begin(),1); it != end(); it++) {
        if (it->second.lru_timestamp_ < v_it->second.lru_timestamp_) {
            v_it = it;
        }
    }
    return v_it;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CacheSet::iterator
CacheSet::rand() {
    return std::next( begin(), GL_RNG_() & (size()-1) );
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CacheSet::iterator
CacheSet::srrip() {
    uint8_t rrpv_jmp = std::numeric_limits<uint8_t>::max();
    for (auto it = begin(); it != end(); it++) {
        if (it->second.rrpv_ == 0) return it;
        rrpv_jmp = std::min(rrpv_jmp, it->second.rrpv_);
    }
    for (auto it = begin(); it != end(); it++) {
        it->second.rrpv_ -= rrpv_jmp;
    }
    return srrip();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CacheSet::iterator
CacheSet::prowb() {
    auto v_it = lru();
    if (v_it->second.dirty_) {
        for (auto it = begin(); it != end(); it++) {
            if (!it->second.dirty_
                && (v_it->second.dirty_ || it->second.lru_timestamp_ < v_it->second.lru_timestamp_))
            {
                v_it = it;
            }
        }
    }
    return v_it;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline uint64_t
_next(const std::deque<uint64_t>& q) {
    return q.empty() ? std::numeric_limits<uint64_t>::max() : q.front();
}

CacheSet::iterator
CacheSet::belady() {
    auto v_it = begin();
    uint64_t v_when = _next(belady_access_stream_.at(v_it->first));
    for (auto it = std::next(begin(), 1); it != end(); it++) {
        uint64_t when = _next(belady_access_stream_.at(it->first));
        if (when > v_when) {
            v_it = it;
        }
    }
    return v_it;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
