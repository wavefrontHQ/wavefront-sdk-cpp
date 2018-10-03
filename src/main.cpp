#include <iostream>
#include <thread>

#include "proxy/WavefrontProxyClient.h"
#include "direct_ingestion/WavefrontDirectIngestionClient.h"

int main(int argc, char const *argv[]) {
    using namespace wavefront;

    std::string wavefrontServer = argv[1];
    std::string token = argv[2];
    std::string proxyHost = argv[3];
    std::string metricsPort = argc < 5 ? "" : argv[4];
    std::string distributionPort = argc < 6 ? "" : argv[5];
    std::string tracingPort = argc < 7 ? "" : argv[6];
    // create direct ingestion client
    WavefrontDirectIngestionClient::Builder builder1(wavefrontServer, token);
    WavefrontDirectIngestionClient *client1 = builder1.build();
    client1->start();

    // create proxy client
    WavefrontProxyClient::Builder builder(proxyHost);
    if (!metricsPort.empty()) {
        builder.setMetricsPort(std::stoi(metricsPort));
    }
    if (!distributionPort.empty()) {
        builder.setDistributionPort(std::stoi(distributionPort));
    }
    if (!tracingPort.empty()) {
        builder.setTracingPort(std::stoi(tracingPort));
    }
    WavefrontProxyClient *client = builder.build();


    std::map<std::string, std::string> tags{{"datacenter", "dc88"},
                                            {"region",     "us-west-1"}};
    std::set<HistogramGranularity> histogramGranularities;
    histogramGranularities.insert(HistogramGranularity::MINUTE);
    histogramGranularities.insert(HistogramGranularity::HOUR);

    std::list<std::pair<double, int>> centroids;
    centroids.push_back(std::make_pair(20.1, 32));
    centroids.push_back(std::make_pair(10.9, 20));

    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // increment the counter by one (second)
        client1->sendMetric("china.direct.usage", 42422.0, -1, "serverApp1", tags);
        client->sendMetric("china.power.usage", 12345.0, -1, "serverApp4", tags);
        // client1->sendDistribution("china.direct.latency", centroids, histogramGranularities, -1, "appServer1", tags);
        // client->sendDistribution("china.power.latency", centroids, histogramGranularities, -1, "appServer1", tags);
    }

    // close clients
    client1->close();
    client->close();

    return 0;
}