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
#ifdef IMPLEMENT_LRU_WITH_FIFO
    auto v_it = find(lru_fifo_.front());
    lru_fifo_.pop_front();
#else
    auto v_it = begin();
    for (auto it = std::next(begin(),1); it != end(); it++) {
        if (it->second.lru_timestamp_ < v_it->second.lru_timestamp_) {
            v_it = it;
        }
    }
#endif
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
