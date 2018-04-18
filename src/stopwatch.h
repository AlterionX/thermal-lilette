#ifndef __STOPWATCH_H__
#define __STOPWATCH_H__

#include <chrono>

namespace std {
    namespace chrono {
        class StopWatch {
            bool paused;
            time_point<steady_clock, duration<float>> lc;
            duration<float> elapsed_time;

        public:
            StopWatch(void);

            bool isPaused(void) const { return paused; }
            duration<float> elapsed(void) const;

            void reset(void);
            void toggle(void);
        };
    }
}

#endif