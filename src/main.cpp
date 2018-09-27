#include <iostream>
#include <thread>

#include "proxy/WavefrontProxyClient.h"
#include "direct_ingestion/WavefrontDirectIngestionClient.h"

int main() {
    using namespace wavefront;

//    WavefrontProxyClient::Builder builder("127.0.0.1");
//    WavefrontProxyClient *client = builder.setMetricsPort(2878).setDistributionPort(40000).build();

    WavefrontDirectIngestionClient::Builder builder1("127.0.0.1:8080", "60bf14c9-ece8-4bea-994e-6353a7e94b1f");
    WavefrontDirectIngestionClient *client1 = builder1.build();
    client1->start();

    std::map<std::string, std::string> tags{{"datacenter", "dc88"},
                                            {"region",     "us-west-1"}};
    std::set<HistogramGranularity> histogramGranularities;
    histogramGranularities.insert(HistogramGranularity::MINUTE);
    histogramGranularities.insert(HistogramGranularity::HOUR);

    std::list<std::pair<double, int>> centroids;
    centroids.push_back(std::make_pair(20.1, 32));
    centroids.push_back(std::make_pair(10.9, 20));
//
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // increment the counter by one (second)
        client1->sendMetric("china.direct.usage", 42422.0, -1, "serverApp1", tags);
        // client->sendMetric("china.power.usage", 12345.0, -1, "serverApp1", tags);
        client1->sendDistribution("china.direct.latency", centroids, histogramGranularities, -1,  "appServer1", tags);
    }
}