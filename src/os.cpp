/*
 *  author: Suhas Vittal
 *  date:   11 October 2024
 * */

#include "os.h"
#include "utils/bitcount.h"

#include <iostream>

#include <stdlib.h>
#include <string.h>

constexpr size_t RAND_MAP_TRIES = 2048;

OS::OS(uint64_t dram_size_mb)
    :num_frames_( (1024*1024*dram_size_mb) / OS_PAGESIZE )
{
    avail_frames_ = new uint64_t[num_frames_ >> 6];
    memset(avail_frames_, 0, sizeof(uint64_t)*(num_frames_>>6));

    avail_frames_[0] = 1;  // Do not allocate page-0 to anyone.
}

OS::~OS() {
    delete[] avail_frames_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

uint64_t
OS::p2v(uint64_t lineaddr) {
    uint64_t pfn, off;
    get_page_and_offset(lineaddr, pfn, off);
    return join_page_and_offset( pfn_to_vpn_.at(pfn), off );
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

uint64_t
OS::v2p(uint64_t lineaddr) {
    uint64_t vpn, off;
    get_page_and_offset(lineaddr, vpn, off);

    if (!vpn_to_page_.count(vpn)) {
        // Make new virtual page.
        OSPage pg;
#ifdef COMPRESSION_TRACES
        memset(pg.data_, 0, 4096);
#endif
        map_page(pg);
        vpn_to_page_[vpn] = pg;
        ++s_virtual_pages_;
    }
    const OSPage& pg = vpn_to_page_.at(vpn);
    return join_page_and_offset( pg.pfn_, off );
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
OS::print_stats(std::ostream& out) {
    PRINT_STAT(out, "OS_MAPPED_PAGES", s_virtual_pages_);
    PRINT_STAT(out, "OS_TOTAL_PAGE_FRAMES", num_frames_);
    out << "\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
OS::map_page(OSPage& pg) {
    ++s_page_faults_;
    for (size_t i = 0; i < RAND_MAP_TRIES; i++) {
        uint64_t pfn = rand() % num_frames_;

        size_t ii = pfn >> 6;
        size_t off = pfn & 63;
        if ( !(avail_frames_[ii] & (1L<<off)) ) {
            avail_frames_[ii] |= (1L<<off);
            pg.pfn_ = pfn;
            return;
        }
    }
    std::cerr << "Failed to map page (total frames = " << num_frames_ << ").\n";
    exit(1);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
get_page_and_offset(uint64_t lineaddr, uint64_t& p, uint64_t& off) {
    p = lineaddr >> Log2<LINES_PER_PAGE>::value;
    off = lineaddr & (LINES_PER_PAGE-1);
}

uint64_t
join_page_and_offset(uint64_t p, uint64_t off) {
    return (p << Log2<LINES_PER_PAGE>::value) | off;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
