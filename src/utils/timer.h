#ifndef UTILS_TIMER_h
#define UTILS_TIMER_h

#include <stdint.h>
#include <time.h>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct Timer {
    struct timespec t_start_;
    struct timespec t_end_;

    void        start(void);
    uint64_t    end(void);
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

inline void
Timer::start() {
    clock_gettime(CLOCK_MONOTONIC_RAW, &t_start_);
}

inline uint64_t
Timer::end() {
    constexpr uint64_t B = 1'000'000'000;

    clock_gettime(CLOCK_MONOTONIC_RAW, &t_end_);
    // Compute difference in times.
    uint64_t time = (B*t_end_.tv_sec + t_end_.tv_nsec) - (B*t_start_.tv_sec + t_start_.tv_nsec);
    return time;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif
