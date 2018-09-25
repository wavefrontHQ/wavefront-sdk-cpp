#include <iostream>
#include <algorithm>

#include "direct_ingestion/DirectIngestionClient.h"
#include "common/Serializer.h"

namespace wavefront {
    const static int BAD_REQUEST = 400;

    // Use those formats to send metric/histogram/tracing data to Wavefront
    const static std::string WAVEFRONT_METRIC_FORMAT = "wavefront";
    const static std::string WAVEFRONT_HISTOGRAM_FORMAT = "histogram";
    const static std::string WAVEFRONT_TRACING_SPAN_FORMAT = "trace";

    DirectIngestionClient::DirectIngestionClient(DirectIngestionClient::Builder *builder) : maxQueueSize(
            builder->maxQueueSize), batchSize(builder->batchSize), flushIntervalSeconds(builder->flushIntervalSeconds),
                                                                                            service(builder->serverName,
                                                                                                    builder->token) {
    }

    int DirectIngestionClient::getFailureCount() {
        return failures.load();
    }

    void DirectIngestionClient::sendDistribution(const std::string &name, std::list<std::pair<double, int>> centroids,
                                                 std::set<wavefront::HistogramGranularity> histogramGranularities,
                                                 long timestamp, const std::string &source,
                                                 std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::histogramToLineData(name, centroids, histogramGranularities, timestamp,
                                                                   (source.empty() ? defaultSource : source), tags);
            // grab the mutex
            std::lock_guard<std::mutex> lock{mutex};
            if (histogramBuffer.size() >= maxQueueSize) {
                std::cerr << "Buffer full, dropping histogram: " << lineData << std::endl;
            } else {
                histogramBuffer.push(lineData);
            }
        } catch (std::invalid_argument e) {
            failures.fetch_add(1);
            std::cerr << e.what() << std::endl;
        }
    }

    void DirectIngestionClient::sendMetric(const std::string &name, double value, long timestamp,
                                           const std::string &source, std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::metricsToLineData(name, value, timestamp,
                                                                 (source.empty() ? defaultSource : source), tags);
            // grab the mutex
            std::lock_guard<std::mutex> lock{mutex};
            if (metricsBuffer.size() >= maxQueueSize) {
                std::cerr << "Buffer full, dropping metrics: " << lineData << std::endl;
            } else {
                metricsBuffer.push(lineData);
            }
        } catch (std::invalid_argument e) {
            failures.fetch_add(1);
            std::cerr << e.what() << std::endl;
        }
    }

    void DirectIngestionClient::sendSpan(const std::string &name, long startMillis, long durationMillis,
                                         boost::uuids::uuid traceId, boost::uuids::uuid spanId,
                                         const std::string &source, std::list<boost::uuids::uuid> parents,
                                         std::list<boost::uuids::uuid> followsFrom,
                                         std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::spanToLineData(name, startMillis, durationMillis, traceId, spanId,
                                                              (source.empty() ? defaultSource : source), parents,
                                                              followsFrom, tags);
            // grab the mutex
            std::lock_guard<std::mutex> lock{mutex};
            if (tracingBuffer.size() >= maxQueueSize) {
                std::cerr << "Buffer full, dropping span: " << lineData << std::endl;
            } else {
                tracingBuffer.push(lineData);
            }
        } catch (std::invalid_argument e) {
            failures.fetch_add(1);
            std::cerr << e.what() << std::endl;
        }
    }

    void DirectIngestionClient::internalFlush(std::queue<std::string> &buffer, const std::string &format) {
        if (buffer.empty())
            return;
        // to decrease contention, using copy buffer
        std::list<std::string> copy_buffer;
        int size = std::min((int) buffer.size(), batchSize);

        mutex.lock();
        for (int i = 0; i < size; i++) {
            copy_buffer.emplace_back(buffer.front());
            buffer.pop();
        }
        mutex.unlock();

        cpr::Response response = service.report(format, copy_buffer);
        // report error
        if (response.status_code >= BAD_REQUEST) {
            failures.fetch_add(1);
            // add back if report failed
            mutex.lock();
            for (auto &element : copy_buffer) {
                buffer.push(element);
            }
            mutex.unlock();
            std::cerr << "Error reporting points, respStatus = " + response.error.message << std::endl;
        } else {
            std::cout << "report points succeed" << std::endl;
        }
    }

    void DirectIngestionClient::flush() {
        internalFlush(metricsBuffer, WAVEFRONT_METRIC_FORMAT);
        internalFlush(histogramBuffer, WAVEFRONT_HISTOGRAM_FORMAT);
        internalFlush(tracingBuffer, WAVEFRONT_TRACING_SPAN_FORMAT);
    }

    void DirectIngestionClient::flushTask() {
        while (true) {
            std::cout << "inside flush()" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(flushIntervalSeconds));
            flush();
        }
    }

    void DirectIngestionClient::start() {
        t = std::thread(&DirectIngestionClient::flushTask, this);
    }

    void DirectIngestionClient::close() {
        // Flush before closing
        flush();
        t.join();
    }
}