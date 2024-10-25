#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <vector>
#include "common.h"
#include "configuration.h"
#include "simple_stats.h"

namespace dramsim3 {

// [MIRZA]
struct MIRZA_Q_Entry {
    uint32_t rowid;
    uint16_t groupid;
    uint16_t actctr;

    std::string to_string() const
    {
        return "RowID: " + std::to_string(rowid) + " GroupID: " + std::to_string(groupid) + " ActCtr: " + std::to_string(actctr);
    }
};

class BankState {
   public:
    BankState(const Config& config, SimpleStats& simple_stats, int rank, int bank_group, int bank);

    enum class State { OPEN, CLOSED, SREF, PD, SIZE };
    Command GetReadyCommand(const Command& cmd, uint64_t clk) const;

    // Update the state of the bank resulting after the execution of the command
    void UpdateState(const Command& cmd, uint64_t clk);

    // Update the existing timing constraints for the command
    void UpdateTiming(const CommandType cmd_type, uint64_t time);

    bool IsRowOpen() const { return state_ == State::OPEN; }
    int OpenRow() const { return open_row_; }
    int RowHitCount() const { return row_hit_count_; }

    std::string StateToString(State state) const;
    void PrintState() const;

    bool CheckAlert();

   private:
    const Config& config_;
    SimpleStats& simple_stats_;

    // Current state of the Bank
    // Apriori or instantaneously transitions on a command.
    State state_;

    // Earliest time when the particular Command can be executed in this bank
    std::vector<uint64_t> cmd_timing_;

    // Currently open row
    int open_row_;

    // consecutive accesses to one row
    int row_hit_count_;

    // rank, bank group, bank
    int rank_;
    int bank_group_;
    int bank_;

    // [RFM] RAA counter (Rolling Accumulated ACT)
    int raa_ctr_;

    // Activations
    std::string acts_stat_name_;
    int acts_counter_;

    // [PRAC] Counters
    std::vector<uint16_t> prac_;

    // [REF]
    uint32_t ref_idx_;

    // [MIRZA]
    void mirza_act(uint32_t rowid);
    void mirza_refresh();
    void mirza_mitig();
    std::vector<uint16_t> mirza_gct_;
    std::vector<MIRZA_Q_Entry> mirza_q_;
    uint32_t mirza_group_size;

    // [MIRZA] Tracking the GCT of the current group being refreshed
    uint32_t mirza_curr_gidx_;
    uint32_t mirza_curr_gct_;

    // [MOAT]
    void moat_act(uint32_t rowid);
    void moat_refresh();
    void moat_mitig();
    int moat_max_prac_idx_;
};

}  // namespace dramsim3
#endif
