/*
 *  author: Suhas Vittal
 *  date:   31 October 2024
 * */

#include "dram/bank.h"
#include "dram/subchannel.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DRAMBank::DRAMBank(const DRAMConfig& conf)
    :conf_(conf)
{}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline uint64_t cap_add(uint64_t tbase, uint64_t tlat) {
    uint64_t t = std::max(tbase, GL_dram_cycle_);
    return tbase + tlat;
}

uint64_t
DRAMBank::execute_command(DRAMCommandType cmd_type) {
    uint64_t finish_dram_cycle;
    switch (cmd_type) {
    case DRAMCommandType::READ:
        finish_dram_cycle = cap_add(next_column_access_ok_cycle_, conf.CL);
        break;
    case DRAMCommandType::WRITE:
        finish_dram_cycle = cap_add(next_column_access_ok_cycle_, conf.CWL);
        break;
    case DRAMCommandType::PRECHARGE:
        finish_dram_cycle = cap_add(next_precharge_ok_cycle_, conf.tRP);
        next_activate_ok_cycle_ = finish_dram_cycle;
        break;
    case DRAMCommandType::ACTIVATE:
        finish_dram_cycle = cap_add(next_activate_ok_cycle_, conf.tRCD);
        next_column_access_ok_cycle_ = finish_dram_cycle;
        next_precharge_ok_cycle_ = cap_add(next_activate_ok_cycle_, conf.tRAS);
        break;
    case DRAMCommandType::READ_PRECHARGE:
        return do_access(DRAMCommandType::READ) + do_access(DRAMCommandType::PRECHARGE);
    case DRAMCommandType::WRITE_PRECHARGE:
        return do_access(DRAMCommandType::WRITE) + do_access(DRAMCommandType::PRECHARGE);
    case DRAMCommandType::REFRESH:
        busy_with_ref_until_dram_cycle_ = GL_dram_cycle_ + conf.tRFC;
        finish_dram_cycle = GL_dram_cycle_ + conf.tRFC;
        break;
    }
    return finish_dram_cycle - GL_dram_cycle_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
