#pragma once

#include <string>

namespace wavefront {
    namespace constant {
        /**
        * Use this format to send metric data to Wavefront
        */
        const static std::string WAVEFRONT_METRIC_FORMAT = "wavefront";

        /**
         * Use this format to send histogram data to Wavefront
         */
        const static std::string WAVEFRONT_HISTOGRAM_FORMAT = "histogram";

        /**
         * Use this format to send tracing data to Wavefront
         */
        const static std::string WAVEFRONT_TRACING_SPAN_FORMAT = "trace";

        /**
         * ∆: INCREMENT
         */
        const static std::string DELTA_PREFIX = "\u2206";

        /**
         * Δ: GREEK CAPITAL LETTER DELTA
         */
        const static std::string DELTA_PREFIX_2 = "\u0394";

        enum class StatusCode {
            OK = 200,
            CREATED = 201,
            ACCEPTED = 202,
            BAD_REQUEST = 400
        };
    }
}

