#include "stopwatch.h"

#include <chrono>

namespace std {
    namespace chrono {
        StopWatch::StopWatch() : paused(true), lc(steady_clock::now()), elapsed_time(duration<double, std::ratio<1, 1000>>::zero()) {}
        duration<float> StopWatch::elapsed(void) const {
            if (paused) return elapsed_time;
            else return elapsed_time + (steady_clock::now() - lc);
        }
        void StopWatch::reset(void) {
            paused = true;
            elapsed_time = duration<float>::zero();
        }
        void StopWatch::toggle(void) {
            paused = !paused;
            if (paused) elapsed_time += steady_clock::now() - lc;
            else lc = steady_clock::now();
        }
    }
}