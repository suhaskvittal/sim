#include "bankstate.h"

namespace dramsim3 {

BankState::BankState(const Config& config, SimpleStats& simple_stats, int rank, int bank_group, int bank)
    :   config_(config),
        simple_stats_(simple_stats),
        state_(State::CLOSED),
        cmd_timing_(static_cast<int>(CommandType::SIZE)),
        open_row_(-1),
        row_hit_count_(0),
        rank_(rank),
        bank_group_(bank_group),
        bank_(bank),
        raa_ctr_(0),
        acts_counter_(0),
        prac_(config_.rows, 0),
        ref_idx_(0)
{
    cmd_timing_[static_cast<int>(CommandType::READ)] = 0;
    cmd_timing_[static_cast<int>(CommandType::READ_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::ACTIVATE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::REFRESH_BANK)] = 0;
    cmd_timing_[static_cast<int>(CommandType::REFsb)] = 0;
    cmd_timing_[static_cast<int>(CommandType::REFab)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SREF_ENTER)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SREF_EXIT)] = 0;
    cmd_timing_[static_cast<int>(CommandType::RFMsb)] = 0; // [RFM] Same Bank RFM
    cmd_timing_[static_cast<int>(CommandType::RFMab)] = 0; // [RFM] All Bank RFM

    // [Stats]
    acts_stat_name_ = "acts." + std::to_string(rank_) + "." + std::to_string(bank_group_) + "." + std::to_string(bank_);    
    simple_stats_.InitStat(acts_stat_name_, "counter", "ACTs Counter");

    // [MIRZA]
    if (config_.mirza_mode == 1)
    {
        mirza_gct_.resize(config_.mirza_groups, 0);
        mirza_group_size = config_.rows / config_.mirza_groups;

        mirza_curr_gidx_ = 0;
        mirza_curr_gct_ = 0;
    }
    else if (config_.moat_mode == 1)
    {
        moat_max_prac_idx_ = -1;
    }
}


Command BankState::GetReadyCommand(const Command& cmd, uint64_t clk) const {
    CommandType required_type = CommandType::SIZE;
    switch (state_) {
        case State::CLOSED:
            switch (cmd.cmd_type) {
                // The state is closed and we need to activate the row to read/write
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                    // [RFM] Block ACTs if RAA Counter == RAAMMT
                    // Don't block all banks, just the bank that wants to launch the command
                    if (config_.rfm_mode == 1 && raa_ctr_ >= config_.raammt)
                    {
                        required_type = CommandType::RFMsb;
                    }
                    else if (config_.rfm_mode == 2 && raa_ctr_ >= config_.raammt)
                    {
                        required_type = CommandType::RFMab;
                    }
                    else
                    {
                        required_type = CommandType::ACTIVATE;
                    }
                    break;
                // The state is closed so we can issue and refresh commands
                case CommandType::REFRESH_BANK:
                case CommandType::REFsb:
                case CommandType::REFab:
                case CommandType::SREF_ENTER:
                case CommandType::RFMsb: // [RFM] Same Bank RFM
                case CommandType::RFMab: // [RFM] All Bank RFM
                    required_type = cmd.cmd_type;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::OPEN:
            switch (cmd.cmd_type) {
                // The state is open and we get a RB hit if the row is the same else we PRECHARGE first
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                    if (cmd.Row() == open_row_)
                    {
                        required_type = cmd.cmd_type;
                    }
                    else
                    {
                        required_type = CommandType::PRECHARGE;
                    }
                    break;
                // The state is open and a precharge command is issued to closed the row to perform refresh
                // TODO: Use PREsb and PREab here
                case CommandType::REFRESH_BANK:
                case CommandType::REFsb:
                case CommandType::REFab:  
                case CommandType::SREF_ENTER:
                case CommandType::RFMsb: // [RFM] Same Bank RFM
                case CommandType::RFMab: // [RFM] All Bank RFM
                    required_type = CommandType::PRECHARGE;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::SREF:
            switch (cmd.cmd_type) {
                // The state is SREF and to read/write we need to exit SREF using SREF_EXIT
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                    required_type = CommandType::SREF_EXIT;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::PD:
        case State::SIZE:
            std::cerr << "In unknown state" << std::endl;
            AbruptExit(__FILE__, __LINE__);
            break;
    }

    if (required_type != CommandType::SIZE)
    {
        if (clk >= cmd_timing_[static_cast<int>(required_type)])
        {
            return Command(required_type, cmd.addr, cmd.hex_addr);
        }
    }
    return Command();
}


// 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, >= 1024
uint32_t get_geostat_bin(uint16_t val)
{
    if (val == 0) return 0;
    if (val == 1) return 1;
    if (val <= 2) return 2;
    if (val <= 4) return 3;
    if (val <= 8) return 4;
    if (val <= 16) return 5;
    if (val <= 32) return 6;
    if (val <= 64) return 7;
    if (val <= 128) return 8;
    if (val <= 256) return 9;
    if (val <= 512) return 10;
    if (val <= 1024) return 11;
    return 12;
}

void BankState::UpdateState(const Command& cmd, uint64_t clk) {
    switch (state_) {
        case State::OPEN:
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::WRITE:
                    row_hit_count_++;
                    break;
                // The state was open and a precharge command was issued which closed the row
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                    state_ = State::CLOSED;
                    open_row_ = -1;
                    row_hit_count_ = 0;
                    break;
                case CommandType::ACTIVATE:
                case CommandType::REFRESH_BANK:
                case CommandType::REFsb:
                case CommandType::REFab:
                case CommandType::SREF_ENTER:
                case CommandType::SREF_EXIT:
                case CommandType::RFMsb: // [RFM] Same Bank RFM
                case CommandType::RFMab: // [RFM] All Bank RFM
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::CLOSED:
            switch (cmd.cmd_type) {
                case CommandType::REFab:
                case CommandType::REFRESH_BANK:
                case CommandType::REFsb:
                    // [RFM]
                    raa_ctr_ -= std::min<int>(raa_ctr_, config_.ref_raa_decrement);

                    // [Stats]
                    acts_counter_ = 0;

                    // [PRAC]
                    for (int i = 0; i < config_.rows_refreshed; i++)
                    {
                        int idx = (ref_idx_ + i) % config_.rows;
                        simple_stats_.IncrementVec("prac_per_tREFI", get_geostat_bin(prac_[idx]));
                        prac_[idx] = 0;
                    }

                    // [MIRZA] Before updating ref_idx_
                    mirza_refresh();

                    // [MOAT]
                    moat_refresh();

                    ref_idx_ = (ref_idx_ + config_.rows_refreshed) % config_.rows;
                    break;
                case CommandType::RFMab: // [RFM] All Bank RFM
                    // [MIRZA]
                    mirza_mitig();

                    // [MOAT]
                    moat_mitig();
                case CommandType::RFMsb: // [RFM] Same Bank RFM, cannot be used with PRAC+ABO
                    raa_ctr_ -= std::min<int>(raa_ctr_, config_.rfm_raa_decrement);
                    break;
                // The state was closed and an activate command was issued which opened the row
                case CommandType::ACTIVATE:
                    state_ = State::OPEN;
                    open_row_ = cmd.Row();

                    // [Stats]
                    acts_counter_++;
                    simple_stats_.Increment(acts_stat_name_);

                    // [RFM]
                    raa_ctr_++;

                    // [PRAC]                    
                    prac_[open_row_]++;

                    // [MIRZA]
                    mirza_act(open_row_);

                    // [MOAT]
                    moat_act(open_row_);
                    break;
                // The state was closed and a refresh command was issued which changed the state to SREF
                case CommandType::SREF_ENTER:
                    state_ = State::SREF;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                case CommandType::SREF_EXIT:
                default:
                    std::cout << cmd << std::endl;
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::SREF:
            switch (cmd.cmd_type) {
                // The state was SREF and a SREF_EXIT command was issued which changed the state to closed
                case CommandType::SREF_EXIT:
                    state_ = State::CLOSED;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::ACTIVATE:
                case CommandType::PRECHARGE:
                case CommandType::REFRESH_BANK:
                case CommandType::REFsb:
                case CommandType::REFab:
                case CommandType::SREF_ENTER:
                case CommandType::RFMsb: // [RFM] Same Bank RFM
                case CommandType::RFMab: // [RFM] All Bank RFM
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return;
}

void BankState::UpdateTiming(CommandType cmd_type, uint64_t time) {
    cmd_timing_[static_cast<int>(cmd_type)] =
        std::max(cmd_timing_[static_cast<int>(cmd_type)], time);
    return;
}

std::string BankState::StateToString(State state) const {
    switch (state) {
        case State::OPEN:
            return "OPEN";
        case State::CLOSED:
            return "CLOSED";
        case State::SREF:
            return "SREF";
        case State::PD:
            return "PD";
        case State::SIZE:
            return "SIZE";
    }
    return "UNKNOWN";
}

void BankState::PrintState() const {
    std::cout << "State: " << StateToString(state_) << std::endl;
    std::cout << "Open Row: " << open_row_ << std::endl;
    std::cout << "Row Hit Count: " << row_hit_count_ << std::endl;
    std::cout << "RAA Counter: " << raa_ctr_ << std::endl;

    // Print the timing constraints
    for (int i = 0; i < static_cast<int>(CommandType::SIZE); i++) {
        std::cout << "Command: " << CommandTypeToString(static_cast<CommandType>(i)) << " Time: " << cmd_timing_[i] << std::endl;
    }
}

bool BankState::CheckAlert()
{
    if (config_.mirza_mode == 1)
    {
        auto found = std::find_if(mirza_q_.begin(), mirza_q_.end(), [this] (const MIRZA_Q_Entry& entry) {
            return entry.actctr >= config_.mirza_qth; // mirza_qth = 40
        });
        
        if (found != mirza_q_.end())
        {
            return true;
        }
        else if (mirza_q_.size() >= config_.mirza_qsize) // mirza_qsize = 4
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (config_.moat_mode == 1)
    {
        if (moat_max_prac_idx_ != -1 and prac_[moat_max_prac_idx_] > config_.moatth)
        {
            return true;
        }
        else 
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// [MIRZA]
void BankState::mirza_act(uint32_t rowid)
{
    if (config_.mirza_mode == 0) return;

    // [MIRZA]
    uint32_t group = rowid / mirza_group_size;
    auto before = mirza_gct_[group]++;

    // std::cout << acts_stat_name_ << "[mirza_act] rowid=" << rowid << " group=" << group << " mirza_gct_=" << mirza_gct_[group] << " before=" << before << " mirza_curr_gidx_=" << mirza_curr_gidx_ << " mirza_curr_gct_=" << mirza_curr_gct_ << std::endl;

    // print mirza_gct_
    // std::cout << "[mirza_act] mirza_gct_" << std::endl;
    // for (auto& entry : mirza_gct_)
    // {
    //     std::cout << entry << " ";
    // }
    // std::cout << std::endl;

    if (group == mirza_curr_gidx_)
    {
        mirza_curr_gct_++;
        if (mirza_curr_gct_ <= config_.mirza_groupth)
        {
            return;
        }
    }
    else if (mirza_gct_[group] <= config_.mirza_groupth)
    {
        return;
    }
    
    // if (!mirza_q_.empty())
    // {
    //     std::cout << "[mirza_act] mirza_q_" << std::endl;
    //     for (auto& entry : mirza_q_)
    //     {
    //         std::cout << entry.to_string() << std::endl;
    //     }
    // }

    auto found = std::find_if(mirza_q_.begin(), mirza_q_.end(), [rowid](const MIRZA_Q_Entry& entry) {
        return entry.rowid == rowid;
    });


    if (found != mirza_q_.end())
    {
        found->actctr++;
        // std::cout << "[mirza_act] found rowid=" << rowid << " actctr=" << found->actctr << std::endl;
        return;
    }

    double targ_prob = 1.0 / config_.mirza_mintw;
    double randval = static_cast<double>(rand() % (1 << 20)) / static_cast<double>(1 << 20);
    if (randval < targ_prob)
    {
        MIRZA_Q_Entry entry;
        entry.rowid = rowid;
        entry.groupid = group;
        entry.actctr = 1;
        mirza_q_.push_back(entry);
    }
}

void BankState::mirza_refresh()
{
    if (config_.mirza_mode == 0) return;

    if (ref_idx_ % mirza_group_size != 0)
    {
        return;
    }

    uint32_t groupid = ref_idx_ / mirza_group_size;
    mirza_curr_gidx_ = groupid;
    mirza_curr_gct_ = mirza_gct_[groupid];
    mirza_gct_[groupid] = 0;

    // std::cout << acts_stat_name_ << "[mirza_refresh] ref_idx=" << ref_idx_ << " groupid=" << groupid << std::endl;

    // if (!mirza_q_.empty())
    // {
    //     std::cout << "[mirza_refresh] before mirza_q_" << std::endl;
    //     for (auto& entry : mirza_q_)
    //     {
    //         std::cout << entry.to_string() << std::endl;
    //     }
    // }
    // [Hrit] Not sure if we should do this?
    mirza_q_.erase(std::remove_if(mirza_q_.begin(), mirza_q_.end(), [groupid] (const MIRZA_Q_Entry& entry) {
        return entry.groupid == groupid;
    }), mirza_q_.end());

    // if (!mirza_q_.empty())
    // {
    //     std::cout << "[mirza_refresh] after mirza_q_" << std::endl;
    //     for (auto& entry : mirza_q_)
    //     {
    //         std::cout << entry.to_string() << std::endl;
    //     }
    // }
}

void BankState::mirza_mitig()
{
    if (config_.mirza_mode == 0) return;

    // std::cout << acts_stat_name_ << "[mirza_mitig] mirza_q_" << std::endl;
    // for (auto& entry : mirza_q_)
    // {
    //     std::cout << entry.to_string() << std::endl;
    // }

    auto max_entry = std::max_element(mirza_q_.begin(), mirza_q_.end(), [] (const MIRZA_Q_Entry& a, const MIRZA_Q_Entry& b) {
        return a.actctr < b.actctr;
    });

    if (max_entry != mirza_q_.end())
    {
        // std::cout << "[mirza_mitig] max_entry rowid=" << max_entry->rowid << " actctr=" << max_entry->actctr << std::endl;
        mirza_q_.erase(max_entry);
    }
}


// [MOAT]
void BankState::moat_act(uint32_t rowid)
{
    if (config_.moat_mode == 0) return;

    if (moat_max_prac_idx_ == -1)
    {
        moat_max_prac_idx_ = rowid;
    }
    else if (prac_[rowid] > prac_[moat_max_prac_idx_])
    {
        moat_max_prac_idx_ = rowid;
    }
}

void BankState::moat_refresh()
{
    if (config_.moat_mode == 0) return;

    // If the moat_max_prac_idx_ or moat_cur_mitig_idx_ is refresed, reset them
    if ((moat_max_prac_idx_ >= ref_idx_) and (moat_max_prac_idx_ < (ref_idx_ + config_.rows_refreshed)))
    {
        moat_max_prac_idx_ = -1;
    }
}

void BankState::moat_mitig()
{
    if (config_.moat_mode == 0) return;

    if (moat_max_prac_idx_ == -1)
    {
        return;
    }

    prac_[moat_max_prac_idx_] = 0;
    
    if (moat_max_prac_idx_ > 0) prac_[moat_max_prac_idx_ - 1]++;
    if (moat_max_prac_idx_ > 1) prac_[moat_max_prac_idx_ - 2]++;
    if (moat_max_prac_idx_ < config_.rows - 1) prac_[moat_max_prac_idx_ + 1]++;
    if (moat_max_prac_idx_ < config_.rows - 2) prac_[moat_max_prac_idx_ + 2]++;

    moat_max_prac_idx_ = -1;
}

}  // namespace dramsim3
