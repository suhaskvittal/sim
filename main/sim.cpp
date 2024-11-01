/*
 *  author: Suhas Vittal
 *  date:   11 October 2024
 * */

#include <defs.h>

#include <core.h>
#include <cache/controller/llc2.h>
#include <os.h>

#ifdef USE_DRAMSIM3
#include <ds3/interface.h>
#else
#include <dram/controller.h>
#include <dram/config.h>
#endif

#include <utils/argparse.h>
#include <utils/timer.h>

#include <iostream>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

constexpr uint64_t  SEED = 12345678;

constexpr uint64_t  DRAM_SIZE_MB = CHANNEL_SIZE_MB * NUM_CHANNELS;
constexpr double    DS3_CLK_SCALE = (4.0/2.4) - 1.0;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

uint64_t    GL_cycle_ = 0;

OS*             GL_os_;
Core*           GL_cores_[N_THREADS];
LLC2Controller* GL_llc_controller_;

#ifdef USE_DRAMSIM3
DS3Interface*   GL_memory_controller_;
#else
DRAMController* GL_memory_controller_;
DRAMConfig      GL_dram_conf_;
#endif

std::mt19937_64 GL_RNG_(SEED);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void init_globals(void);
void cleanup(void);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void print_sim_config(void);
void print_progress(void);
void print_announcement(std::string);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

std::string OPT_trace_file_;
std::string OPT_ds3_cfg_;
uint64_t OPT_num_inst_;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    /*
     * Parse arguments:
     * */
    ArgParseResult ARGS = parse(argc, argv,
            { // REQUIRED
                "trace",
            },
            { // OPTIONAL
                { "ds3cfg", "DRAMSim3 config file (*.ini)", "../../ds3conf/base.ini" },
                { "inst", "Number of instructions to simulate", "10000000" }
            });
    ARGS("trace", OPT_trace_file_);
    ARGS("ds3cfg", OPT_ds3_cfg_);
    ARGS("inst", OPT_num_inst_);

    init_globals();
    /*
     * Start simulation.
     * */
    bool all_done;
    size_t first = 0;
    double leap_op = 0.0;

    print_sim_config();
    print_announcement("SIMULATION START");

    uint64_t t_ns_spent_in_core = 0,
             t_ns_spent_in_mem = 0;
    Timer tt;
    do {
        // Print progress.
        if (GL_cycle_ % 1'000'000 == 0) {
            print_progress();
        }

        all_done = true;

#ifdef USE_DRAMSIM3
        if (leap_op >= 1.0) {
            leap_op -= 1.0;
        } else {
            tt.start();
            GL_memory_controller_->mem_->ClockTick();
            t_ns_spent_in_mem += tt.end();
            leap_op += DS3_CLK_SCALE;
        }
#else
        tt.start();
        GL_memory_controller_->tick();
        t_ns_spent_in_mem += tt.end();
#endif

        tt.start();
        
        GL_llc_controller_->tick();
        size_t ii = first;
        for (size_t i = 0; i < N_THREADS; i++) {
            Core* c = GL_cores_[ii];
            c->tick();
            all_done &= (c->finished_inst_num_ >= OPT_num_inst_);
            ii = INCREMENT_AND_MOD_BY_POW2(ii, N_THREADS);
        }
        first = INCREMENT_AND_MOD_BY_POW2(first, N_THREADS);

        t_ns_spent_in_core += tt.end();

        ++GL_cycle_;
    } while (!all_done);

    print_announcement("SIMULATION END");

    PRINT_STAT(std::cout, "SIM_TIME_IN_CORE", t_ns_spent_in_core/1e9);
    PRINT_STAT(std::cout, "SIM_TIME_IN_MEM", t_ns_spent_in_mem/1e9);
    PRINT_STAT(std::cout, "SYS_CYCLES", GL_cycle_);
    
    std::cout << "\n";

    cleanup();

    return 0;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void
init_globals() {
    for (size_t i = 0; i < N_THREADS; i++) {
        GL_cores_[i] = new Core(i, 4);
        GL_cores_[i]->set_trace_file(OPT_trace_file_);
    }
    GL_os_ = new OS(DRAM_SIZE_MB);
    GL_llc_controller_ = new LLC2Controller;
#ifdef USE_DRAMSIM3
    GL_memory_controller_ = new DS3Interface(OPT_ds3_cfg_);
#else
    GL_memory_controller_ = new DRAMController;
    fill_config_for_4400_4800_5200(GL_dram_conf_);
#endif
}

void
cleanup() {
    for (size_t i = 0; i < N_THREADS; i++) {
        GL_cores_[i]->print_stats(std::cout);
    }
    GL_os_->print_stats(std::cout);
    GL_llc_controller_->print_stats(std::cout);
#ifdef USE_DRAMSIM3
    GL_memory_controller_->print_stats();
#else
    GL_memory_controller_->print_stats(std::cout);
#endif

    for (size_t i = 0; i < N_THREADS; i++) {
        delete GL_cores_[i];
    }
    delete GL_os_;
    delete GL_llc_controller_;
    delete GL_memory_controller_;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

template <class T>
inline void list(std::string name, T value) {
    std::cout << std::setw(24) << std::left << name << ":\t" << value << "\n";
}

void
print_sim_config() {
    std::cout << "\n---------------------------------------------\n\n";

    list("TRACE", OPT_trace_file_);
    list("DS3CFG", OPT_ds3_cfg_);
    list("INST", FMT_BIGNUM(OPT_num_inst_));

    std::cout << "\n---------------------------------------------\n\n";

    list("N_THREADS", N_THREADS);

    std::cout << "\n---------------------------------------------\n\n";

    list("LLC_SIZE_KB", LLC_SIZE_KB);
    list("LLC_ASSOC", LLC_ASSOC);
    list("LLC_REPL_POLICY", repl_policy_name(LLC_REPL_POLICY));

    std::cout << "\n---------------------------------------------\n\n";
    
    list("DRAM_CHANNELS", NUM_CHANNELS);
    list("DRAM_SUBCHANNELS", NUM_SUBCHANNELS);
    list("DRAM_RANKS", NUM_RANKS);
    list("DRAM_BANKGROUPS", NUM_BANKGROUPS);
    list("DRAM_BANKS", NUM_BANKS);
    list("DRAM_ROWS", NUM_ROWS);

    list("DRAM_TOTAL_SIZE_MB", CHANNEL_SIZE_MB*NUM_CHANNELS);
    list("DRAM_RANK_SIZE_MB", RANK_SIZE_MB);
    list("DRAM_BANK_SIZE_MB", BANK_SIZE_MB);
    list("DRAM_ROW_SIZE", NUM_COLUMNS*COLUMN_WIDTH/8);

    std::cout << "\n---------------------------------------------\n\n";

    list("tCAS (ns)", GL_dram_conf_.CL * GL_dram_conf_.tCK);
    list("tRCD (ns)", GL_dram_conf_.tRCD * GL_dram_conf_.tCK);
    list("tRP (ns)", GL_dram_conf_.tRP * GL_dram_conf_.tCK);
    list("tRAS (ns)", GL_dram_conf_.tRAS * GL_dram_conf_.tCK);
    list("tRFC (ns)", GL_dram_conf_.tRFC * GL_dram_conf_.tCK);
    list("tREFI (us)", GL_dram_conf_.tREFI * GL_dram_conf_.tCK * 1e-3);


    std::cout << "\n---------------------------------------------\n\n";

    list("ROB_WIDTH", ROB_WIDTH);
    list("OS_PAGESIZE", OS_PAGESIZE);
    list("LINESIZE", LINESIZE);

    std::cout << "\n---------------------------------------------\n\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void 
print_progress() {
    if (GL_cycle_ % 50'000'000 == 0) {
        std::cout << "\nCYCLE = " << std::setw(4) << std::left << FMT_BIGNUM(GL_cycle_) << " [ INST:";
        for (size_t i = 0; i < N_THREADS; i++) {
            std::cout << std::setw(7) << std::right << FMT_BIGNUM(GL_cores_[i]->finished_inst_num_);
        }
        std::cout << " ]\n\tprogress: ";
    }
    std::cout << ".";
    std::cout.flush();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline std::string
get_bar(size_t n) {
    std::stringstream ss;
    while (n--) ss << "-";
    return ss.str();
}

void 
print_announcement(std::string a) {
    constexpr size_t BAR_SIZE = 80;
    std::string full_bar = get_bar(BAR_SIZE);
    std::string half_bar = get_bar( (BAR_SIZE-a.size())/2 - 1 );

    std::cout << "\n\n" << full_bar 
                << "\n" << half_bar << " " << a << " " << half_bar 
                << "\n" << full_bar << "\n\n";
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
