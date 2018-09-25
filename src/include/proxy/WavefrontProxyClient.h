#pragma once

#include "proxy/ProxyConnectionHandler.h"
#include "common/WavefrontSender.h"

namespace wavefront {
    /**
    * WavefrontProxyClient that sends data directly via TCP to the Wavefront Proxy Agent.
    * @author Mengran Wang (mengranw@vmware.com)
    */
    class WavefrontProxyClient : public WavefrontSender {
    public:
        // nested class for client builder
        struct Builder {
            Builder(const std::string &hostName) : hostName(hostName) {
            }

            Builder setMetricsPort(unsigned short metricsPort) {
                this->metricsPort = metricsPort;
                return *this;
            }

            Builder setTracingPort(unsigned short tracingPort) {
                this->tracingPort = tracingPort;
                return *this;
            }

            Builder setDistributionPort(unsigned short distributionPort) {
                this->distributionPort = distributionPort;
                return *this;
            }

            WavefrontProxyClient *build() {
                return new WavefrontProxyClient(this);
            }

            std::string hostName;
            unsigned short metricsPort = 0;
            unsigned short distributionPort = 0;
            unsigned short tracingPort = 0;
        };

        void sendMetric(const std::string &name, double value, long timestamp = -1, const std::string &source = "",
                        std::map<std::string, std::string> tags = {{}}) override;

        void sendDistribution(const std::string &name, std::list<std::pair<double, int>> centroids,
                              std::set<HistogramGranularity> histogramGranularities, long timestamp = -1,
                              const std::string &source = "",
                              std::map<std::string, std::string> tags = {{}}) override;

        void sendSpan(const std::string &name, long startMillis, long durationMillis,
                      boost::uuids::uuid traceId, boost::uuids::uuid spanId, const std::string &source = "",
                      std::list<boost::uuids::uuid> parents = {}, std::list<boost::uuids::uuid> followsFrom = {},
                      std::map<std::string, std::string> tags = {{}}) override;


        int getFailureCount();

        void close();

    private:
        WavefrontProxyClient(Builder *builder);

        std::unique_ptr<ProxyConnectionHandler> metricHandler = nullptr;
        std::unique_ptr<ProxyConnectionHandler> distributionHandler = nullptr;
        std::unique_ptr<ProxyConnectionHandler> tracingHandler = nullptr;
        // source is hardcoded
        std::string defaultSource = "wavefrontProxySender";
    };
}