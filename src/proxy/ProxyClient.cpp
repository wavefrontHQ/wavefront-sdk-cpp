#include "proxy/ProxyClient.h"
#include "common/Serializer.h"

#include <iostream>
namespace wavefront {

    ProxyClient::ProxyClient(ProxyClient::Builder *builder) {
        if (builder->distributionPort != 0) {
            distributionHandler = std::unique_ptr<ProxyConnectionHandler>(
                    new ProxyConnectionHandler(builder->hostName, builder->distributionPort));
            distributionHandler->connect();
        }

        if (builder->metricsPort != 0) {
            metricHandler = std::unique_ptr<ProxyConnectionHandler>(
                    new ProxyConnectionHandler(builder->hostName, builder->metricsPort));
            metricHandler->connect();
        }

        if (builder->tracingPort != 0) {
            tracingHandler = std::unique_ptr<ProxyConnectionHandler>(
                    new ProxyConnectionHandler(builder->hostName, builder->tracingPort));
            tracingHandler->connect();
        }
        // hardcoded
        defaultSource = "localhost";
    }

    int ProxyClient::getFailureCount() {
        int result = 0;
        if (metricHandler != nullptr) {
            result += metricHandler->getFailureCount();
        }

        if (distributionHandler != nullptr) {
            result += distributionHandler->getFailureCount();
        }

        if (tracingHandler != nullptr) {
            result += tracingHandler->getFailureCount();
        }

        return result;
    }

    void ProxyClient::close() {
        if (metricHandler != nullptr) {
            try {
                metricHandler->close();
            } catch (SocketException e) {
                metricHandler->incrementFailureCount();
                std::cerr << e.what() << std::endl;
            }
        }

        if (distributionHandler != nullptr) {
            try {
                distributionHandler->close();
            } catch (SocketException e) {
                distributionHandler->incrementFailureCount();
                std::cerr << e.what() << std::endl;
            }
        }

        if (tracingHandler != nullptr) {
            try {
                tracingHandler->close();
            } catch (SocketException e) {
                tracingHandler->incrementFailureCount();
                std::cerr << e.what() << std::endl;
            }
        }
    }

    void ProxyClient::sendMetric(const std::string &name, double value, long timestamp, const std::string &source,
                                 std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::metricsToLineData(name, value, timestamp,
                                                                 (source.empty() ? defaultSource : source), tags);
            metricHandler->sendData(lineData);
        } catch (SocketException e) {
            metricHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        } catch (std::invalid_argument e) {
            metricHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        }
    }

    void ProxyClient::sendDistribution(const std::string &name, std::list<std::pair<double, int>> centroids,
                                       std::set<wavefront::HistogramGranularity> histogramGranularities, long timestamp,
                                       const std::string &source, std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::histogramToLineData(name, centroids, histogramGranularities, timestamp,
                                                                   (source.empty() ? defaultSource : source), tags);
            distributionHandler->sendData(lineData);
        } catch (SocketException e) {
            distributionHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        } catch (std::invalid_argument e) {
            distributionHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        }
    }

    void ProxyClient::sendSpan(const std::string &name, long startMillis, long durationMillis,
                               boost::uuids::uuid traceId, boost::uuids::uuid spanId, const std::string &source,
                               std::list<boost::uuids::uuid> parents, std::list<boost::uuids::uuid> followsFrom,
                               std::map<std::string, std::string> tags) {
        try {
            std::string lineData = Serializer::spanToLineData(name, startMillis, durationMillis, traceId, spanId,
                                                              (source.empty() ? defaultSource : source), parents,
                                                              followsFrom, tags);
            tracingHandler->sendData(lineData);
        } catch (SocketException e) {
            tracingHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        } catch (std::invalid_argument e) {
            tracingHandler->incrementFailureCount();
            std::cerr << e.what() << std::endl;
        }
    }


}