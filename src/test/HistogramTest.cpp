/*
 * @Author: Sangtian Wang
 * @Date: 2019-11-13 21:52:57
 * @LastEditTime: 2019-11-15 11:44:42
 * @FilePath: /wavefront-sdk-cpp/src/HistogramTest.cpp
 */

/* NOTE: to run test FASTER, you can change MIN_IN_MILLI to 2000 in Histogram.h */

#include "gtest/gtest.h"
#include "common/Histogram.h"
#include <unistd.h>
#include <thread>
#include <chrono>

#define DELTA 1e-1f /* DELTA for error */

/* Insert power of 10 values to WavefrontHistogram*/
void CreatePow10Histogram(wavefront::WavefrontHistogram &w_h) {
    w_h.Update(0.1);
    w_h.Update(1.0);
    w_h.Update(1e1);
    w_h.Update(1e1);
    w_h.Update(1e2);
    w_h.Update(1e3);
    w_h.Update(1e4);
    w_h.Update(1e4);
    w_h.Update(1e5);
}

/* Insert 1 to 100 values to WavefrontHistogram*/
void Inc100Histogram(wavefront::WavefrontHistogram &w_h) {
    for (int i=1; i<101; i++) {
        w_h.Update(double(i));
    }
}

/* Insert 1 to 1000 values to WavefrontHistogram*/
void Inc1000Histogram(wavefront::WavefrontHistogram &w_h) {
    for (int i=1; i<1001; i++) {
        w_h.Update(double(i));
    }
}

/**
 * @description: Return Distributions in unordered_map format.
 * @param distributions: vector of distributions
 * @return: unordered_map key = means, value = weights
 */
std::unordered_map<double, double> DistributionToMap(std::vector<wavefront::Distribution> distributions) {
    std::unordered_map<double, double> dist_map;
    for (auto &tmp: distributions) {
        for (std::pair<double, double> &p: tmp.centroids) {
            dist_map[p.first] += p.second;
        }
    }
    return dist_map;
}

/* Test distribution. */
TEST(HistogramTest, Distribution) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));

    std::vector<wavefront::Distribution> distributions = w_h.FlushDistribution();
    std::unordered_map<double, double> dist_map = DistributionToMap(distributions);
    EXPECT_EQ(dist_map.size(), 7);
    EXPECT_NEAR(dist_map[0.1], 1, DELTA);
    EXPECT_NEAR(dist_map[1.0], 1, DELTA);
    EXPECT_NEAR(dist_map[1e1], 2, DELTA);
    EXPECT_NEAR(dist_map[1e2], 1, DELTA);
    EXPECT_NEAR(dist_map[1e3], 1, DELTA);
    EXPECT_NEAR(dist_map[1e4], 2, DELTA);
    EXPECT_NEAR(dist_map[1e5], 1, DELTA);

    EXPECT_EQ(w_h.GetCount(), 0);
    EXPECT_TRUE(isnan(w_h.GetMax()));
    EXPECT_TRUE(isnan(w_h.GetMin()));
    EXPECT_TRUE(isnan(w_h.GetMean()));
    EXPECT_NEAR(w_h.GetSum(), 0, DELTA);

    wavefront::Snapshot snapshot = w_h.GetSnapshot();
    EXPECT_EQ(snapshot.GetCount(), 0);
    EXPECT_TRUE(isnan(snapshot.GetMax()));
    EXPECT_TRUE(isnan(snapshot.GetMin()));
    EXPECT_TRUE(isnan(snapshot.GetMean()));
    EXPECT_EQ(snapshot.GetSum(), 0);
    EXPECT_TRUE(isnan(snapshot.GetValue(0.5)));
}

/* Test bulk update. */
TEST(HistogramTest, BulkUpdate) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    std::vector<double> means = {24.2, 84.35, 1002.0};
    std::vector<double> weights = {80, 1, 9};

    w_h.BulkUpdate(means, weights);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    std::vector<wavefront::Distribution> distributions = w_h.FlushDistribution();
    std::unordered_map<double, double> dist_map = DistributionToMap(distributions);

    EXPECT_EQ(dist_map.size(), 3);
    EXPECT_NEAR(dist_map[24.2], 80, DELTA);
    EXPECT_NEAR(dist_map[84.35], 1, DELTA);
    EXPECT_NEAR(dist_map[1002.0], 9, DELTA);
}

/* Test get sum of distribution. */
TEST(HistogramTest, TestSum) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetSum(), 121121.1, DELTA);
    EXPECT_NEAR(w_h.GetSnapshot().GetSum(), 121121.1, DELTA);
}

/* Test get max of distribution. */
TEST(HistogramTest, TestMax) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetMax(), 1e5, DELTA);
    EXPECT_NEAR(w_h.GetSnapshot().GetMax(), 1e5, DELTA);
}

/* Test get min of distribution. */
TEST(HistogramTest, TestMin) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetMin(), 0.1, DELTA);
    EXPECT_NEAR(w_h.GetSnapshot().GetMin(), 0.1, DELTA);
}

/* Test get mean of distribution. */
TEST(HistogramTest, TestMean) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetMean(), 13457.9, DELTA);
    EXPECT_NEAR(w_h.GetSnapshot().GetMean(), 13457.9, DELTA);
    
}

/* Test get std dev of distribution. */
TEST(HistogramTest, TestStdDev) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetStdDev(), 30859.85, DELTA);
    cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
    
    wavefront::WavefrontHistogram w_h_100(cur_time_millis);
    Inc100Histogram(w_h_100);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h_100.GetStdDev(), 28.87, DELTA);
    
    wavefront::WavefrontHistogram w_h_1000(cur_time_millis);
    Inc1000Histogram(w_h_1000);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h_1000.GetStdDev(), 288.67, DELTA);
}

/* Test get size of distribution. */
TEST(HistogramTest, TestSize) {
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    CreatePow10Histogram(w_h);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
    EXPECT_NEAR(w_h.GetSnapshot().GetCount(), 9, DELTA);
}


/* Multi-threading test helper function*/
void test_bulk_update(wavefront::WavefrontHistogram &w_h, std::vector<double> means, std::vector<double> weights) {
    w_h.BulkUpdate(means, weights);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));
}

/* Test of multi-threading case. */
TEST(HistogramTest, MultThread){
    uint64_t cur_time_millis = std::chrono::duration_cast< std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    wavefront::WavefrontHistogram w_h(cur_time_millis);
    std::vector<double> means1 = {21.2, 82.35, 1042.0};
    std::vector<double> weights1 = {70, 2, 6};
    std::vector<double> means2 = {24.2, 84.35, 1002.0};
    std::vector<double> weights2 = {80, 1, 9};
    std::vector<double> means3 = {21.2, 84.35, 1052.0};
    std::vector<double> weights3 = {60, 12, 8};
    
    test_bulk_update(w_h, means1, weights1);
    std::thread t1(test_bulk_update, std::ref(w_h), means2, weights2);
    std::thread t2(test_bulk_update, std::ref(w_h), means3, weights3);
    std::this_thread::sleep_for (std::chrono::seconds(MIN_IN_MILLI/1000+1));

    std::vector<wavefront::Distribution> distributions = w_h.FlushDistribution();
    std::unordered_map<double, double> dist_map = DistributionToMap(distributions);
    
    t1.join();
    t2.join();

    EXPECT_EQ(dist_map.size(), 7);
    EXPECT_NEAR(dist_map[21.2], 130, DELTA);
    EXPECT_NEAR(dist_map[24.2], 80, DELTA);
    EXPECT_NEAR(dist_map[82.35], 2, DELTA);
    EXPECT_NEAR(dist_map[84.35], 13, DELTA);
    EXPECT_NEAR(dist_map[1002.0], 9, DELTA);
    EXPECT_NEAR(dist_map[1042.0], 6, DELTA);
    EXPECT_NEAR(dist_map[1052.0], 8, DELTA);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}