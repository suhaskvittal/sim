/*
 *  author: Suhas Vittal
 *  date:   24 October 2024
 * */

#include "cache.h"

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
ssrip(CacheSet& s) {
    return s.begin();
}

////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

CacheSet::iterator
lru_wb(CacheSet& s) {
    auto v_it = lru(s);
    if (v_it->second.dirty_) {
        // Find alternative line to evict.
        for (auto it = s.begin(); it != s.end(); it++) {
            if ( !it->second.dirty_ 
                && (v_it->second.dirty_ || it->second.lru_timestamp_ < v_it->second.lru_timestamp_) ) 
            {
                v_it = it;
            }
        }
    }
    return v_it;
}

////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
