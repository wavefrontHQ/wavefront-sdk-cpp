# wavefront-sdk-cpp
Wavefront by VMware SDK for C++ is the core library for sending metrics, histograms and trace data from your C++ application to Wavefront using either a Wavefront proxy or direct ingestion.

## Requirements
The only explicit requirements are:

* A C++11 compatible compiler such as Clang or GCC. 
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

## Set Up a Wavefront Sender

You can choose to send metrics, histograms, or trace data from your application to the Wavefront service using one of the following techniques:
* Use [direct ingestion](https://docs.wavefront.com/direct_ingestion.html) to send the data directly to the Wavefront service. This is the simplest way to get up and running quickly.
* Use a [Wavefront proxy](https://docs.wavefront.com/proxies.html), which then forwards the data to the Wavefront service. This is the recommended choice for a large-scale deployment that needs resilience to internet outages, control over data queuing and filtering, and more.

Create a Wavefront sender that corresponds to your choice:
* Option 1: [Create a `WavefrontDirectIngestionClient`](#option-1-create-a-wavefrontdirectingestionclient) to send data directly to a Wavefront service.
* Option 2: [Create a `WavefrontProxyClient`](#option-2-create-a-wavefrontproxyclient) to send data to a Wavefront proxy.

### Option 1. Create a WavefrontDirectIngestionClient
To create a `WavefrontDirectIngestionClient`, you build it with the information it needs to send data directly to Wavefront.

#### Step 1. Obtain Wavefront Access Information
Gather the following access information:

* Identify the URL of your Wavefront instance. This is the URL you connect to when you log in to Wavefront, typically something like `https://<domain>.wavefront.com`.
* In Wavefront, verify that you have Direct Data Ingestion permission, and [obtain an API token](http://docs.wavefront.com/wavefront_api.html#generating-an-api-token).

#### Step 2. Initialize the WavefrontDirectIngestionClient
Initialize a `WavefrontDirectIngestionClient` by building it with the required URL and token you obtained in Step 1.

You can optionally call builder methods to tune the following ingestion properties:

* Max queue size - Internal buffer capacity of the `WavefrontSender`. Any data in excess of this size is dropped.
* Flush interval - Interval for flushing data from the `WavefrontSender` directly to Wavefront.
* Batch size - Amount of data to send to Wavefront in each flush interval.

Together, the batch size and flush interval control the maximum theoretical data throughput. You should override the defaults _only_ to set higher values.


```cpp
#include "direct_ingestion/WavefrontDirectIngestionClient.h"

// Create a builder with the URL of the form "https://DOMAIN.wavefront.com"
// and a Wavefront API token with direct ingestion permission granted
WavefrontDirectIngestionClient::Builder directBuilder(<server>, <token>);

// Optionally configure the builder to override default tuning properties
//   Max queue size (in data points). Default: 50000 
//   Batch Size (in data points). Default: 10000
//   Flush interval (in seconds). Default: 1 second 
directBuilder.setMaxQueueSize(100000).setBatchSize(20000).setFlushingInterval(2);

// Build the Wavefront sender
WavefrontDirectIngestionClient *wavefrontSender = directBuilder.build();

// Explicitly start the flushing thread (required for direct ingestion)
wavefrontSender->start();
```

### Option 2: Create a `WavefrontProxyClient`

**Note:** Before your application can use a `WavefrontProxyClient`, you must [set up and start a Wavefront proxy](https://github.com/wavefrontHQ/java/tree/master/proxy#set-up-a-wavefront-proxy).

To create a `WavefrontProxyClient`, you build it with the information it needs to send data to the Wavefront proxy, including:

* The name of the host that will run the Wavefront proxy.
* One or more proxy listening ports to send data to. The ports you specify depend on the kinds of data you want to send (metrics, histograms, and/or trace data). You must specify at least one listener port.
* Optional settings for tuning communication with the proxy.


```cpp
#include "proxy/WavefrontProxyClient.h"

// Create a builder with the proxy hostname or address
WavefrontProxyClient::Builder proxyBuilder(<host>);

// Configure the builder with proxy listening ports:
//   the default listener port (2878) for sending metrics to
//   the recommended port (2878) for sending histograms to
//   the recommended port (30000) for sending trace data to
// And then build the WavefrontProxyClient
WavefrontProxyClient *wavefrontSender = proxyBuilder.setMetricsPort(2878).
                                        setDistributionPort(2878).
                                        setTracingPort(30000).
                                        build();
```

**Note:** When you [set up a Wavefront proxy](https://github.com/wavefrontHQ/java/tree/master/proxy#set-up-a-wavefront-proxy) on the specified proxy host, you specify the port it will listen to for each type of data to be sent. The `WavefrontProxyClient` must send data to the same ports that the Wavefront proxy listens to. Consequently, the port-related builder methods must specify the same port numbers as the corresponding proxy configuration properties:

| `WavefrontProxyClient` builder method | Corresponding property in `wavefront.conf` |
| ----- | -------- |
| `setMetricsPort()` | `pushListenerPorts=` |
| `setDistributionPort()` | `histogramDistListenerPorts=` |
| `setTracingPort()` | `traceListenerPorts=` |

## Send Data to Wavefront

You send a data point to Wavefront by calling a method on the Wavefront sender you built.
  
**Note:** Direct ingestion usually sends data in batch and compressed format. 

### Metrics and Delta Counters


```cpp
// Wavefront Metrics Data format
// <metricName> <metricValue> [<timestamp>] source=<source> [pointTags]
//
// Example: "new-york.power.usage 42422 1533529977 source=localhost datacenter=dc1"
//
wavefrontSender->sendMetric("new-york.power.usage", 42422.0, 1533529977L,
    "localhost", {{"datacenter", "dc1"}});
```
### Distributions (Histograms)

```cpp
//  Wavefront Histogram Data format
//  {!M | !H | !D} [<timestamp>] #<count> <mean> [centroids] <histogramName> source=<source> [pointTags]
// 
//  Example: You can choose to send to atmost 3 bins: Minute, Hour, Day
//  "!M 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
//  "!H 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
//  "!D 1533529977 #20 30.0 #10 5.1 request.latency source=appServer1 region=us-west"
// 
std::set<HistogramGranularity> histogramGranularities;
histogramGranularities.insert(HistogramGranularity::MINUTE);
histogramGranularities.insert(HistogramGranularity::HOUR);
histogramGranularities.insert(HistogramGranularity::DAY);

std::list<std::pair<double, int>> centroids;
centroids.push_back(std::make_pair(20.1, 32));
centroids.push_back(std::make_pair(10.9, 20));
    
wavefrontSender->sendDistribution("request.latency", centroids, histogramGranularities, 
    1533529977L, "appServer1", {{"region", "west"}});
```

### Tracing Spans
```cpp
//  Wavefront Tracing Span Data format
//  <tracingSpanName> source=<source> [pointTags] <start_millis> <duration_milliseconds>
// 
//  Example: "getAllUsers source=localhost
//            traceId=7b3bf470-9456-11e8-9eb6-529269fb1459
//            spanId=0313bafe-9457-11e8-9eb6-529269fb1459
//            parent=2f64e538-9457-11e8-9eb6-529269fb1459
//            application=Wavefront http.method=GET
//            1552949776000 343"
// 
boost::uuids::string_generator string_gen;
wavefrontSender->sendSpan("getAllUsers",1552949776000L, 343L, "localhost",
      string_gen("7b3bf470-9456-11e8-9eb6-529269fb1459"),
      string_gen("0313bafe-9457-11e8-9eb6-529269fb1459"),
      {string_gen("2f64e538-9457-11e8-9eb6-529269fb1459")}, 
      {},
      {{"application", "Wavefront"}, {"http.method", "GET"}};
```


## Close the Wavefront Sender

Remember to flush the buffer and close the Wavefront sender before shutting down your application.

```cpp
// If there are any failures observed while sending metrics/histograms/tracing-spans above, 
// you can get the total failure count using the below API
int total_failures = wavefrontSender->getFailureCount();
  
// Close existing proxy connections, or 
// flush current buffers and stop the flushing thread for direct ingestion
wavefrontSender->close();
```
