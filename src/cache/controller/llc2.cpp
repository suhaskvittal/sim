/*
 *  author: Suhas Vittal
 *  date:   28 October 2024
 * */

#include "cache/controller/llc2.h"
#include "core.h"

#ifdef USE_DRAMSIM3
#include "ds3/interface.h"
#else
#include "dram/controller.h"
#endif

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
LLC2Controller::update_prev_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t when) {
    GL_cores_[coreid]->rob_[robid].end_cycle_ = when;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int
LLC2Controller::access_next_level(uint64_t lineaddr, size_t coreid, size_t robid, uint64_t inst_num, bool is_load) {
    return GL_memory_controller_->make_request(lineaddr, is_load) ? 1 : -1;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
LLC2Controller::_tick() {
}

void
LLC2Controller::_print_stats(std::ostream& out) {
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
