#include <iostream>
#include <algorithm>

#include "direct_ingestion/WavefrontDirectIngestionClient.h"
#include "common/Serializer.h"
#include "common/Constants.h"

namespace wavefront {

    WavefrontDirectIngestionClient::WavefrontDirectIngestionClient(WavefrontDirectIngestionClient::Builder *builder)
            : maxQueueSize(
            builder->maxQueueSize), batchSize(builder->batchSize), flushIntervalSeconds(builder->flushIntervalSeconds),
              service(builder->serverName,
                      builder->token) {
    }

    int WavefrontDirectIngestionClient::getFailureCount() {
        return failures.load();
    }

    void WavefrontDirectIngestionClient::sendDistribution(const std::string &name,
                                                          std::list<std::pair<double, int>> centroids,
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

    void WavefrontDirectIngestionClient::sendMetric(const std::string &name, double value, long timestamp,
                                                    const std::string &source,
                                                    std::map<std::string, std::string> tags) {
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

    void WavefrontDirectIngestionClient::sendSpan(const std::string &name, long startMillis, long durationMillis,
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

    void WavefrontDirectIngestionClient::internalFlush(std::queue<std::string> &buffer, const std::string &format) {
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
        if (response.status_code != static_cast<int>(constant::StatusCode::OK) &&
            response.status_code != static_cast<int>(constant::StatusCode::ACCEPTED)) {
            failures.fetch_add(1);
            // add back if report failed
            mutex.lock();
            for (auto &element : copy_buffer) {
                buffer.push(element);
            }
            mutex.unlock();
            std::cerr << "Error reporting points, respStatus = " + response.error.message << std::endl;
        } else {
            std::cout << "report points succeed: " << response.status_code << std::endl;
        }
    }

    void WavefrontDirectIngestionClient::flush() {
        internalFlush(metricsBuffer, constant::WAVEFRONT_METRIC_FORMAT);
        internalFlush(histogramBuffer, constant::WAVEFRONT_HISTOGRAM_FORMAT);
        internalFlush(tracingBuffer, constant::WAVEFRONT_TRACING_SPAN_FORMAT);
    }

    void WavefrontDirectIngestionClient::flushTask() {
        while (is_running) {
            std::this_thread::sleep_for(std::chrono::seconds(flushIntervalSeconds));
            flush();
            std::cout << "done flush()" << std::endl;
        }
    }

    void WavefrontDirectIngestionClient::start() {
        // start flushing thread
        is_running.store(true);
        t = std::thread(&WavefrontDirectIngestionClient::flushTask, this);
    }

    void WavefrontDirectIngestionClient::close() {
        // Flush before closing
        flush();
        is_running.store(false);
        t.join();
    }
}