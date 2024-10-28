/*
 *  author: Suhas Vittal
 *  date:   24 October 2024
 * */

#include "cache.h"

#include <algorithm>
#include <limits>

#include <stdlib.h>

CacheSet::iterator
lru(CacheSet& s) {
    auto v_it = s.begin();
    for (auto it = std::next(s.begin(),1); it != s.end(); it++) {
        if (it->second.lru_timestamp_ < v_it->second.lru_timestamp_) {
            v_it = it;
        }
    }
    return v_it;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CacheSet::iterator
rand(CacheSet& s) {
    return std::next( s.begin(), rand() & (s.size()-1) );
}

////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

CacheSet::iterator
srrip(CacheSet& s) {
    uint8_t rrpv_jmp = std::numeric_limits<uint8_t>::max();
    for (auto it = s.begin(); it != s.end(); it++) {
        if (it->second.rrpv_ == 0) return it;
        rrpv_jmp = std::min(rrpv_jmp, it->second.rrpv_);
    }
    for (auto it = s.begin(); it != s.end(); it++) {
        it->second.rrpv_ -= rrpv_jmp;
    }
    return srrip(s);
}

////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
