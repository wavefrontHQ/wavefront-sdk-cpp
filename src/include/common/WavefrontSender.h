#pragma once

#include <map>
#include <string>
#include <list>
#include <set>
#include <boost/uuid/uuid.hpp>
#include "common/HistogramGranularity.h"

namespace wavefront {
    /**
    * An abstract WavefrontSender class that contains various sending methods along with flushing and closing logic
    *
    * @author Mengran Wang (mengranw@vmware.com)
    */
    class WavefrontSender {
    public:
        WavefrontSender() {}

        ~WavefrontSender() {}

        /**
         * Sends the given metric to Wavefront
         *
         * @param name      The name of the metric. Spaces are replaced with '-' (dashes) and quotes will
         *                  be automatically escaped.
         * @param value     The value to be sent.
         * @param timestamp The timestamp in milliseconds since the epoch to be sent. If equal -1 then the
         *                  timestamp is assigned by Wavefront when data is received.
         * @param source    The source (or host) that's sending the metric. If empty then assigned by
         *                  Wavefront.
         * @param tags      The tags associated with this metric.
         */
        virtual void
        sendMetric(const std::string &name, double value, long timestamp = -1, const std::string &source = "",
                   std::map<std::string, std::string> tags = {{}}) = 0;

        /**
        * Sends the given histogram to Wavefront
        * @param name                       The name of the histogram distribution. Spaces are replaced
        *                                   with '-' (dashes) and quotes will be automatically escaped.
        * @param centroids                  The distribution of histogram points to be sent.
        *                                   Each centroid is a 2-dimensional {std::pair} where the
        *                                   first dimension is the mean value (Double) of the centroid
        *                                   and second dimension is the count of points in that centroid.
        * @param histogramGranularities     The set of intervals (minute, hour, and/or day) by which
        *                                   histogram data should be aggregated.
        * @param timestamp                  The timestamp in milliseconds since the epoch to be sent.
        *                                   If equals -1 then the timestamp is assigned by Wavefront when
        *                                   data is received.
        * @param source                     The source (or host) that's sending the histogram. If
        *                                   empty, then assigned by Wavefront.
        * @param tags                       The tags associated with this histogram.
        */
        virtual void sendDistribution(const std::string &name, std::list<std::pair<double, int>> centroids,
                                      std::set<HistogramGranularity> histogramGranularities, long timestamp = -1,
                                      const std::string &source = "",
                                      std::map<std::string, std::string> tags = {{}}) = 0;

        /**
         * Send a trace span to Wavefront.
         *
         * @param name                The operation name of the span.
         * @param startMillis         The start time in milliseconds for this span.
         * @param durationMillis      The duration of the span in milliseconds.
         * @param source              The source (or host) that's sending the span. If empty, then
         *                            assigned by Wavefront.
         * @param traceId             The unique trace ID for the span.
         * @param spanId              The unique span ID for the span.
         * @param parents             The list of parent span IDs, can be null if this is a root span.
         * @param followsFrom         The list of preceding span IDs, can be null if this is a root span.
         * @param tags                The span tags associated with this span. Supports repeated tags.
         */
        virtual void sendSpan(const std::string &name, long startMillis, long durationMillis,
                              boost::uuids::uuid traceId, boost::uuids::uuid spanId, const std::string &source = "",
                              std::list<boost::uuids::uuid> parents = {},
                              std::list<boost::uuids::uuid> followsFrom = {},
                              std::map<std::string, std::string> tags = {{}}) = 0;

        /**
        * Sends the given delta counter to Wavefront. The timestamp for the point on the client side is
        * null because the final timestamp of the delta counter is assigned when the point is
        * aggregated on the server side. Do not use this method to send older points (say around 5 min
        * old) as they will be aggregated on server with the current timestamp which yields in a wrong
        * final aggregated value.
        *
        * @param name      The name of the delta counter. Name will be prefixed by ∆ if it does
        *                  not start with that symbol already. Also, spaces are replaced with '-'
        *                  (dashes) and quotes will be automatically escaped.
        * @param value     The delta value to be sent. This will be aggregated on the Wavefront server
        *                  side.
        * @param source    The source (or host) that's sending the metric. If null then assigned by
        *                  Wavefront.
        * @param tags      The tags associated with this metric.
        */
        virtual void sendDeltaCounter(std::string &name, double value, const std::string &source,
                                      std::map<std::string, std::string> tags = {{}}) = 0;

    };
}