#pragma once

#include <string>
#include <map>
#include <cmath>
#include <sstream>
#include <list>
#include <set>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/HistogramGranularity.h"

namespace wavefront {
    /**
    * Common Util methods to serialize data point
    *
    * @author Mengran Wang(mengranw@vmware.com)
    */
    static const std::string quote = "\"";

    class Serializer {
    public:
        // Write a double as a string, with proper formatting for infinity and NaN
        std::string static toString(double v) {
            if (std::isnan(v)) {
                return "Nan";
            }
            if (std::isinf(v)) {
                return (v < 0 ? "-Inf" : "+Inf");
            }
            return std::to_string(v);
        }

        // Write a HistogramGranularity as a string
        static std::string toString(HistogramGranularity type) {
            switch (type) {
                case HistogramGranularity::MINUTE: {
                    return "!M";
                }
                case HistogramGranularity::HOUR: {
                    return "!H";
                }
                case HistogramGranularity::DAY: {
                    return "!D";
                }
                default:
                    return "Invalid";
            }
        }

        const static std::string escapeCharacter(const std::string &value) {
            bool copy = false;
            std::string temp = "";

            for (size_t i = 0; i < value.size(); ++i) {
                char c = value[i];
                if (c == '\\' || c == '"' || c == '\n') {
                    if (!copy) {
                        temp.reserve(value.size() + 1);
                        temp.assign(value, 0, i);
                        copy = true;
                    }
                    if (c == '\\') {
                        temp.append("\\\\");
                    } else if (c == '"') {
                        temp.append("\\\"");
                    } else {
                        temp.append("\\\n");
                    }
                } else if (copy) {
                    temp.push_back(c);
                }
            }
            return copy ? temp : value;
        }

        void static appendTagMap(std::stringstream &out, std::map<std::string, std::string> tags) {
            if (tags.empty())
                return;

            for (auto &tag : tags) {
                out << " " << quote << escapeCharacter(tag.first) << quote << "=" << quote
                    << escapeCharacter(tag.second)
                    << quote;
            }
        }


        static std::string
        metricsToLineData(const std::string &name, double value, long timestamp, const std::string &source,
                          std::map<std::string, std::string> tags) {
            /*
            * Wavefront Metrics Data format
            * <metricName> <metricValue> [<timestamp>] source=<source> [pointTags]
            *
            * Example: "new-york.power.usage 42422 1533531013 source=localhost datacenter=dc1"
            */
            if (name.empty()) {
                throw std::invalid_argument("metrics name can't be empty");
            }
            std::stringstream ss;

            ss << quote << escapeCharacter(name) << quote << " ";
            ss << toString(value) << " ";
            if (timestamp != -1) {
                ss << (timestamp / 1000) << " ";
            }
            ss << "source=" << quote << escapeCharacter(source) << quote;
            appendTagMap(ss, tags);
            ss << "\n";

            return ss.str();
        }

        static std::string
        histogramToLineData(const std::string &name, std::list<std::pair<double, int>> centroids,
                            std::set<wavefront::HistogramGranularity> histogramGranularities, long timestamp,
                            const std::string &source, std::map<std::string, std::string> tags) {
            if (name.empty()) {
                throw std::invalid_argument("histogram name cannot be blank");
            }

            if (histogramGranularities.empty()) {
                throw std::invalid_argument("Histogram granularities cannot be null or empty");
            }

            if (centroids.empty()) {
                throw std::invalid_argument("A distribution should have at least one centroid");
            }

            std::stringstream ss;
            for (auto &histogramGranularity : histogramGranularities) {
                ss << toString(histogramGranularity) << " ";
                if (timestamp != -1) {
                    ss << (timestamp / 1000) << " ";
                }
                // centroids
                for (auto &centroid : centroids) {
                    ss << "#" << centroid.second << " " << centroid.first << " ";
                }
                // Metric
                ss << quote << escapeCharacter(name) << quote << " ";

                // Source
                ss << "source=" << quote << escapeCharacter(source) << quote;
                appendTagMap(ss, tags);
                ss << "\n";
            }
            return ss.str();
        }

        static std::string spanToLineData(const std::string &name, long startMillis, long durationMillis,
                                   boost::uuids::uuid traceId, boost::uuids::uuid spanId, const std::string &source,
                                   std::list<boost::uuids::uuid> parents, std::list<boost::uuids::uuid> followsFrom,
                                   std::map<std::string, std::string> tags){
            /*
            * Wavefront Tracing Span Data format
            * <tracingSpanName> source=<source> [pointTags] <start_millis> <duration_milli_seconds>
            *
            * Example: "getAllUsers source=localhost
            *           traceId=7b3bf470-9456-11e8-9eb6-529269fb1459
            *           spanId=0313bafe-9457-11e8-9eb6-529269fb1459
            *           parent=2f64e538-9457-11e8-9eb6-529269fb1459
            *           application=Wavefront http.method=GET
            *           1533531013 343500"
            */
            if (name.empty()) {
                throw std::invalid_argument("tracing name cannot be blank");
            }
            std::stringstream ss;
            ss << quote << escapeCharacter(name) << quote << " ";
            // Source
            ss << "source=" << quote << escapeCharacter(source) << quote << " ";
            ss << "traceId=" << boost::uuids::to_string(traceId) << " ";
            ss << "spanId=" << boost::uuids::to_string(spanId) << " ";
            if(!parents.empty()){
                for(auto &parent : parents){
                    ss << "parent=" << boost::uuids::to_string(parent) << " ";
                }
            }

            if(!followsFrom.empty()){
                for(auto &follow : followsFrom){
                    ss << "followsFrom=" << boost::uuids::to_string(follow) << " ";
                }
            }
            appendTagMap(ss, tags);
            ss << " " << startMillis << " " << durationMillis;
            ss << "\n";

            // TODO: support span log
            return ss.str();
        }


    };
}