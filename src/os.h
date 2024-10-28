/*
 *  author: Suhas Vittal
 *  date:   11 October 2024
 * */

#ifndef OS_h
#define OS_h

#include "defs.h"

#include <iostream>
#include <unordered_map>
#include <vector>

#include <stdint.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct OSPage {
    uint64_t pfn_;
#ifdef COMPRESSION_TRACES
    char data_[4096];
    uint64_t s_reads_;
    uint64_t s_writes_;
#endif
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class OS {
public:
    const uint64_t num_frames_;

    uint64_t s_virtual_pages_ =0;
    uint64_t s_page_faults_   =0;
private:
    std::unordered_map<uint64_t, uint64_t> pfn_to_vpn_;
    std::unordered_map<uint64_t, OSPage> vpn_to_page_;
    /*
     * Bitvector of available page frames.
     * */
    uint64_t* avail_frames_;
public:
    OS(uint64_t dram_size_mb);
    ~OS(void);

    uint64_t v2p(uint64_t lineaddr);
    uint64_t p2v(uint64_t lineaddr);

    void print_stats(std::ostream&);
private:
    void map_page(OSPage&);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void     get_page_and_offset(uint64_t lineaddr, uint64_t& page, uint64_t& offset);
uint64_t join_page_and_offset(uint64_t page, uint64_t offset);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // OS_h
