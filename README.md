# wavefront-sdk-cpp
This library provides support for sending metrics, histograms and opentracing spans to Wavefront via proxy or direct ingestion.

## Requirements
The only explicit requirements are:

* a C++11 compatible compiler such as Clang or GCC. The minimum required version of GCC is unknown, so if anyone has trouble building this library with a specific version of GCC, do let me know
* CMake (version >= 3.12)
* Boost (version >= 1.63)
* curl and its development libraries

## Building
```
# fetch third-party dependencies
git submodule init
git submodule update

# compile and build
mkdir -p build
cd build
cmake ..
make -j 2

# to run the example
./src/wavefront-sdk

# to generate shared lib 
make install
```

## Usage

### Create Wavefront Client

#### Create a `WavefrontProxyClient`

Assume you have a running Wavefront proxy listening on at least one of metrics / direct-distribution / tracing ports and you know the proxy hostname.

```cpp
#include "proxy/WavefrontProxyClient.h"
/**
* Create Proxy Client
* host: Hostname of the Wavefront proxy (required)
* metrics_port: Metrics Port on which the Wavefront proxy is listening on
* distribution_port: Distribution Port on which the Wavefront proxy is listening on
* tracing_port: Tracing Port on which the Wavefront proxy is listening on
*/

 WavefrontProxyClient::Builder builder(<host>);
 WavefrontProxyClient *client = builder.setMetricsPort(2878).
                                        setDistributionPort(40000).
                                        setTracingPort(30000).
                                        build();
```

#### Create a `WavefrontDirectIngestionClient`

Assume you have a running Wavefront cluster and you know the server URL (example - https://mydomain.wavefront.com) and the API token.

```cpp
#include "direct_ingestion/WavefrontDirectIngestionClient.h"
/** 
* Create Direct Ingestion Client
* server: Server address, Example: https://mydomain.wavefront.com (required)
* token: Token with Direct Data Ingestion permission granted (required)
* max_queue_size: Max Queue Size, size of internal data buffer for each data type. 50000 by default
* batch_size: Batch Size, amount of data sent by one API call, 10000 by default
* flush_interval_seconds: Buffer flushing interval time, 5 seconds by default
*/

WavefrontDirectIngestionClient::Builder builder1(<server>, <token>);
WavefrontDirectIngestionClient *client1 = builder1.build();

// must be explicitly called, start flushing thread
client1->start();
```

### Send Data Point

Using following functions to send data point to Wavefront via Proxy or Direct ingestion.  
The differences between sending data via Proxy and Direct ingestion are:
* Direct ingestion calls `report` API and usually sends data in batch and compressed format 
```cpp
// 1) Send Metric to Wavefront
  /*
   * Wavefront Metrics Data format
   * <metricName> <metricValue> [<timestamp>] source=<source> [pointTags]
   *
   * Example: "new-york.power.usage 42422 1533529977 source=localhost datacenter=dc1"
   */
  client->sendMetric("new-york.power.usage", 42422.0, 1533529977L,
        "localhost", {{"datacenter", "dc1"}});

  // 2) Send Direct Distribution (Histogram) to Wavefront
  /*
   * Wavefront Histogram Data format
   * {!M | !H | !D} [<timestamp>] #<count> <mean> [centroids] <histogramName> source=<source> 
   * [pointTags]
   *
   * Example: You can choose to send to atmost 3 bins - Minute/Hour/Day
   * 1) Send to minute bin    =>    
   *    "!M 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
   * 2) Send to hour bin      =>    
   *    "!H 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
   * 3) Send to day bin       =>    
   *    "!D 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
   */
    std::set<HistogramGranularity> histogramGranularities;
    histogramGranularities.insert(HistogramGranularity::MINUTE);
    histogramGranularities.insert(HistogramGranularity::HOUR);
    histogramGranularities.insert(HistogramGranularity::DAY);

    std::list<std::pair<double, int>> centroids;
    centroids.push_back(std::make_pair(20.1, 32));
    centroids.push_back(std::make_pair(10.9, 20));
    
    client->sendDistribution("request.latency", centroids, histogramGranularities, 
        1533529977L, "appServer1", {{"region", "west"}});

  // 4) Send OpenTracing Span to Wavefront
  /*
   * Wavefront Tracing Span Data format
   * <tracingSpanName> source=<source> [pointTags] <start_millis> <duration_milliseconds>
   *
   * Example: "getAllUsers source=localhost
   *           traceId=7b3bf470-9456-11e8-9eb6-529269fb1459
   *           spanId=0313bafe-9457-11e8-9eb6-529269fb1459
   *           parent=2f64e538-9457-11e8-9eb6-529269fb1459
   *           application=Wavefront http.method=GET
   *           1533529977 343500"
   */
  boost::uuids::string_generator string_gen;
  client->sendSpan("getAllUsers",1533529977L, 343500L, "localhost",
        string_gen("7b3bf470-9456-11e8-9eb6-529269fb1459"),
        string_gen("0313bafe-9457-11e8-9eb6-529269fb1459"),
        {string_gen("2f64e538-9457-11e8-9eb6-529269fb1459")}, 
        {},
        {{"application", "Wavefront"}, {"http.method", "GET"}};
```


#### Get failure count and close connection

```cpp
// If there are any failures observed while sending metrics/histograms/tracing-spans above, 
// you can get the total failure count using the below API
int total_failures = client->getFailureCount();
  
// For proxy client: 
// close existing connections of the client
// For direct ingestion client:
// If you want to mannally flush current buffers and stop the flushing thread
client->close();
```