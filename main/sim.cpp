/*
 *  author: Suhas Vittal
 *  date:   11 October 2024
 * */

#include <defs.h>

#include <core.h>
#include <cache.h>
#include <cache/controller.h>
#include <ds3/interface.h>
#include <os.h>

#include <utils/timer.h>

#include <iostream>

static const uint64_t   DRAM_SIZE_MB = 32*1024;
static const uint64_t   INST_SIM = 10'000'000;
static const double     DS3_CLK_SCALE = (4.0/2.4) - 1.0;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

uint64_t    GL_cycle_ = 0;
Core        GL_cores_[N_THREADS];

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void print_progress(void);
void print_announcement(std::string);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    std::string trace_file(argv[1]);

    OS* os = new OS(DRAM_SIZE_MB);
    DS3Interface* ds3i = new DS3Interface("../ds3conf/base.ini");
    CacheController<LLC>* llc_ctrl = new CacheController<LLC>(24);

    for (size_t i = 0; i < N_THREADS; i++) {
        GL_cores_[i].os_ = os;
        GL_cores_[i].llc_ctrl_ = llc_ctrl;
        GL_cores_[i].set_trace_file(trace_file);
        GL_cores_[i].coreid_ = i;
        GL_cores_[i].fetch_width_ = 4;
    }

    llc_ctrl->ds3i_ = ds3i;
    ds3i->llc_ctrl_ = llc_ctrl;
    /*
     * Start simulation.
     * */
    bool all_done;
    size_t first = 0;
    double leap_op = 0.0;

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

        if (leap_op >= 1.0) {
            leap_op -= 1.0;
        } else {
            tt.start();
            ds3i->mem_->ClockTick();
            t_ns_spent_in_mem += tt.end();

            leap_op += DS3_CLK_SCALE;
        }

        tt.start();
        
        llc_ctrl->tick();
        size_t ii = first;
        for (size_t i = 0; i < N_THREADS; i++) {
            Core& c = GL_cores_[ii];
            c.tick();
            all_done &= (c.finished_inst_num_ >= INST_SIM);
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

    for (size_t i = 0; i < N_THREADS; i++) {
        GL_cores_[i].print_stats(std::cout);
    }
    llc_ctrl->print_stats(std::cout, "LLC");
    os->print_stats(std::cout);
    ds3i->print_stats();

    delete ds3i;
    delete os;
    delete llc_ctrl;

    return 0;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void 
print_progress() {
    if (GL_cycle_ % 50'000'000 == 0) {
        std::cout << "\nCYCLE = " << std::setw(4) << std::right<< GL_cycle_/1'000'000 << "M [ INST:";
        for (size_t i = 0; i < N_THREADS; i++) {
            std::cout << std::setw(6) << std::right << (GL_cores_[i].finished_inst_num_/1'000'000) << "M";
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
