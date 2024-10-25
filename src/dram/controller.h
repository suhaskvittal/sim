/*
 *  author: Suhas Vittal
 *  date:   11 October 2024
 * */

#ifndef DRAM_CONTROLLER_h
#define DRAM_CONTROLLER_h

#include "dram/channel.h"

struct DRAMRequest {
    bool is_read_;
    uint64_t address_;
    /*
     * Only set for reads.
     * */
    int32_t mshr_id_ =-1;
};

class DRAMController {
public:
    constexpr static RW_QUEUE_SIZE = 128;

    uint64_t cycle_ =0;

    const DRAMConfig& conf_;
    DRAMAddressMapping am_;
private:
    std::vector<DRAMChannel> channels_;

    std::deque<DRAMRequest> read_queue_;
    std::deque<DRAMRequest> write_queue_;
    /*
     * All pending requests are assigned a `reqid_` that is known
     * to its corresponding `CmdQueueEntry`. Once the DRAM command finishes,
     * the pending request is updated and removed.
     * */
    uint64_t reqid_ =0;
    std::unordered_map<uint64_t, DRAMRequest> pending_requests_;
public:
private:
    CmdQueueEntry cmd_from_request(const DRAMRequest&);
};

#endif  // DRAM_CONTROLLER_h
