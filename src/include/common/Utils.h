#pragma once

#include <chrono>

namespace wavefront {
    namespace Utils {
        // clock std::ratio<1, 1000000000> ---> nanoseconds level
        //Elapsed times for it events are measured internally in nanoseconds, its precision and accuracy vary depending on
        // operating system and hardware.
        typedef std::chrono::high_resolution_clock Clock;

        inline long get_millis_from_epoch() {
            auto now = std::chrono::system_clock::now();
            auto since_epoch = now.time_since_epoch(); // get the duration since epoch

            int64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count();
            return millis;
        }

        inline long get_seconds_from_epoch() {
            auto now = std::chrono::system_clock::now();
            auto since_epoch = now.time_since_epoch(); // get the duration since epoch

            int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
            return seconds;
        }
    }
}

