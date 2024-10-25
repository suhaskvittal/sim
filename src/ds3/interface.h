/*
 *  author: Suhas Vittal
 *  date:   15 October 2024
 * */

#ifndef DS3_INTERFACE_h
#define DS3_INTERFACE_h

#include "defs.h"

#include <memory_system.h>

#include <unordered_map>

namespace ds3=dramsim3;
/*
 * Declaration of `CacheController` here
 * */
template <class T> class CacheController;

class DS3Interface {
public:
    ds3::MemorySystem* mem_;
    CacheController<LLC>* llc_ctrl_;

    uint64_t s_queue_full_ =0;
private:
    std::unordered_map<uint64_t, size_t> lineaddr_mshr_map_;
public:
    DS3Interface(std::string ds3conf);
    ~DS3Interface(void);
    /*
     * Returns false if the request could not be made.
     * */
    bool make_request(uint64_t lineaddr, bool is_read, size_t mshr_id);

    void print_stats(void);
};

#endif  // DS3_INTERFACE_h
