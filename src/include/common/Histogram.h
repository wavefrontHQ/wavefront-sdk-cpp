/*
 * @Author: Sangtian Wang
 * @Date: 2019-11-13 21:28:48
 * @LastEditTime: 2019-11-14 17:48:20
 * @LastEditors: Sangtian Wang
 * @FilePath: /wavefront-sdk-cpp/src/include/common/Histogram.h
 */

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <math.h>
#include <thread>
#include <iostream>
#include <queue>
#include <deque>
#include <unordered_map>
#include "TDigest.h"

#define MIN_IN_MILLI 2000

namespace wavefront {

    /* Wrapper for TDigest distribution */
    class Snapshot {
        public:
            /**
             * @description: constructor of Snapshot
             * @param tdigest::TDigest
             * @return: tdigest::TDigest
             */
            Snapshot(tdigest::TDigest t) {snapshot_digest = t;}
            
            Snapshot(const Snapshot &o) {snapshot_digest = o.snapshot_digest;}
            /**
             * @description: get max value in snapshot
             * @return: max value in snapshot
             */
            double GetMax() {
                return snapshot_digest.quantile(1.0);
            }

            /**
             * @description: get min value in snapshot
             * @return: minimum value in snapshot
             */
            double GetMin() {
                return snapshot_digest.quantile(0.0);
            }
            
            /**
             * @description: get mean value in snapshot
             * @return: mean value in snapshot
             */
            double GetMean() {
                return snapshot_digest.getMean();
            }

            /**
             * @description: get sum of snapshot.
             * @return: sum in snapshot.
             */
            double GetSum() {
                return snapshot_digest.getSum();
            }
            
            /**
             * @description: get total weight of snapshot.
             * @return: total weight in snapshot.
             */
            double GetWeight() {
                return snapshot_digest.getWeight();
            }

            /**
             * @description: get # of centroids of snapshot.
             * @return: total # of centroids in snapshot.
             */
            int GetCount() {
                return snapshot_digest.totalSize();
            }

            /**
             * @description: get the value of given quantile.
             * @param quantile, range between 0.0 and 1.0.
             * @return: the value in the distribution at the given quantile, or NAN if 
             * snapshot is empty or param out of range. 
             */
            double GetValue(double quantile) {
                return snapshot_digest.quantile(quantile);
            }

            tdigest::TDigest snapshot_digest;
    };

    class Distribution {
        /**
         * Representation of a histogram distribution.
         *Containing a timestamp and a list of centroids.
         */        
        public:
            Distribution() {};
            
            /**
             * @description: Construct a distribution.
             * @param timestamp: Timestamp in milliseconds since the epoch.
             * @param centroids: Centroids
             */
            Distribution(uint64_t timpstamp_, std::vector<std::pair<double, double>> centroids_) {timestamp = timpstamp_; centroids = centroids_;}
            
            Distribution(const Distribution & o) {timestamp = o.timestamp; centroids = o.centroids;}

            uint64_t timestamp;
            std::vector<std::pair<double, double>> centroids;
    };

    class ThreadMinBin {
        public:
            ThreadMinBin() {}
             /**
             * @description: Construct a ThreadMinBin.
             * @param _accuary: accuary
             *        _minute_millis: minute in millisecond
             */
            ThreadMinBin(double _accuary, uint64_t _minute_millis) {accuracy = _accuary, minute_millis = _minute_millis;};

            ThreadMinBin(const ThreadMinBin &o) {
                accuracy = o.accuracy;
                minute_millis = o.minute_millis;
                for (auto &tmp: o.per_thread_dist) {
                    per_thread_dist[tmp.first] = tmp.second;
                }
            }
            /* Retrieve the thread-local dist in one given minute. */
            tdigest::TDigest *GetDistByThreadID(std::thread::id thread_id){
                if (per_thread_dist.find(thread_id) ==  per_thread_dist.end()){
                    per_thread_dist[thread_id] = tdigest::TDigest(accuracy);
                }
                return &per_thread_dist[thread_id];
            }

            /* Update the value in the distribution of given thread id. */
            void UpdateDistByThreadID(std::thread::id thread_id, double value) {
                GetDistByThreadID(thread_id)->add(value);
            }
            
            /* Bulk update values in the distribution of given thread id. */
            void BulkUpdateDistByThreadID(std::thread::id thread_id, std::vector<double> means, std::vector<double> weights) {
                if (means.size() != weights.size()){
                    return;
                }
                tdigest::TDigest *cur = GetDistByThreadID(thread_id);
                for(int i=0; i<means.size(); i++){
                    cur->add(means[i], weights[i]);
                }
            }

            /* Get list of centroids for dists of all threads in this minute. */
            std::vector<std::vector<tdigest::Centroid>> GetCentroids(){
                std::vector<std::vector<tdigest::Centroid>> all_centroids;
                for(auto &tmp: per_thread_dist) {
                    std::thread::id thread_id = tmp.first;
                    std::vector<tdigest::Centroid> tmp_centroids = per_thread_dist[thread_id].allCentroids();
                    all_centroids.push_back(tmp_centroids);
                }
                return all_centroids;
            }

            /* Convert to Distribution. */
            std::vector<Distribution> ToDistribution() {
                std::vector<Distribution> dist;
                std::vector<std::vector<tdigest::Centroid>> all_centroids = GetCentroids();
                for (auto &tmp: all_centroids) {
                    Distribution per_thread_dist;
                    per_thread_dist.timestamp = minute_millis;
                    for(auto &c: tmp){
                        per_thread_dist.centroids.push_back(std::make_pair(c.mean(), c.weight()));
                    }
                    dist.push_back(per_thread_dist);
                }
                return dist;
            } 

            double accuracy;
            uint64_t minute_millis;

            std::unordered_map<std::thread::id, tdigest::TDigest> per_thread_dist;

    };

    /* Wavefront implementation of a histogram. */
    class WavefrontHistogram {
        public:
            /**
             * @description: Construct Wavefront Histogram.
             * @param cur_min_millis: current miniute in millisecond
             */
            WavefrontHistogram(uint64_t cur_min_millis = 0) {
                if (cur_min_millis == 0) {
                    clock_milli_ = CurrentClockMillis();
                }
                else {
                    clock_milli_ = cur_min_millis;
                }
                cur_minute_bin_ = ThreadMinBin(accuracy, CurrentMinuteMillis());
            }

            WavefrontHistogram(const WavefrontHistogram &o) {
                accuracy = o.accuracy;
                max_bins = o.max_bins;
                clock_milli_ = o.clock_milli_;
                prior_minute_bins_ = o.prior_minute_bins_;
                cur_minute_bin_ = o.cur_minute_bin_;
            }

            /* Get current time in millisecond. */
            uint64_t CurrentClockMillis() {
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
            }

            /* Get the timestamp of start of certain minute. */
            uint64_t CurrentMinuteMillis() {
                return uint64_t(CurrentClockMillis()/MIN_IN_MILLI)*MIN_IN_MILLI;
            }

            /**
             * @description: Add one value to the distribution.
             * @param value to add.
             */
            void Update(double value) {
                std::thread::id this_id = std::this_thread::get_id();
                GetCurrentBin()->UpdateDistByThreadID(this_id, value);
            }

            /**
             * @description: Bulk-update this histogram with a set of centroids.
             * @param means: vector of means
             * @param weights: vector of weights
             */
            void BulkUpdate(std::vector<double> means, std::vector<double> weights) {
                std::thread::id this_id = std::this_thread::get_id();
                ThreadMinBin *cur_bin = GetCurrentBin();
                cur_bin->BulkUpdateDistByThreadID(this_id, means, weights);
            }

            /**
             * @description: Retrieve the current bin.
             * @return: current bin with type ThreadMinuteBin
             */
            ThreadMinBin *GetCurrentBin() {
                return GetOrUpdateCurrentBin(CurrentMinuteMillis());
            }
            
            void UpdatePriorBin() {
                GetOrUpdateCurrentBin(CurrentMinuteMillis());
            }
            /**
             * @description: Get current minute bin. Will update _prior_minute_bins_list if the minute has passed.
             * @param cur_min_millis: Current minute in millis
             * @return: current bin with type ThreadMinuteBin
             */
            ThreadMinBin *GetOrUpdateCurrentBin(uint64_t cur_min_millis) {
                if(cur_minute_bin_.minute_millis == cur_min_millis) {
                    return &cur_minute_bin_;   
                }

                std::lock_guard<std::mutex> lock(mtx_);
                if(prior_minute_bins_.size() == max_bins) {
                    prior_minute_bins_.pop_front();
                }
                prior_minute_bins_.push_back(cur_minute_bin_);
                cur_minute_bin_ = ThreadMinBin(accuracy, cur_min_millis);

                return &cur_minute_bin_;
            }
            
            /* Return newly-updated prior_minute_bins_. */
            std::deque<ThreadMinBin> *GetPriorMinuteBins() {
                GetOrUpdateCurrentBin(CurrentMinuteMillis());
                return &prior_minute_bins_;
            }            
            
            /**
             * @description: Aggregate all the minute bins prior to the current minute.
             * Only aggregating before current minute is because threads might be
             * updating the current minute bin while the method is invoked.
             * @return: a list of the distributions held within each bin.
             */
            std::vector<Distribution> FlushDistribution() {
                std::vector<Distribution> distributions;
                GetOrUpdateCurrentBin(CurrentMinuteMillis());
                
                std::lock_guard<std::mutex> lock(mtx_);
                while(!prior_minute_bins_.empty()) {
                    ThreadMinBin tmp = prior_minute_bins_.back();
                    std::vector<Distribution> dist= tmp.ToDistribution();
                    distributions.insert(distributions.begin(), dist.begin(), dist.end());
                    prior_minute_bins_.pop_back();
                }
                return distributions;
            } 
            
            /**
             * @description: Return a statistical of the histogram distribution.
             * @return: Snapshot of Histogram
             */
            Snapshot GetSnapshot() {
                UpdatePriorBin();
                tdigest::TDigest new_digest(accuracy);
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        new_digest.merge(&d.second);
                    }
                }
                return Snapshot(new_digest);
            }

            /* Get the number of values in the distribution. */
            int GetCount() {
                UpdatePriorBin();
                int count = 0;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        d.second.quantile(-1); //trigger tdigest to process()
                        count += d.second.totalWeight();
                    }
                }
                return count;
            }

            /* Get the sum of the values in the distribution. */
            double GetSum() {
                UpdatePriorBin();
                double sum = 0;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        d.second.quantile(-1); //trigger tdigest to process()
                        sum += d.second.getSum();
                    }
                }
                return sum;
            }
            
            /* Get the maximum value in the distribution. Return NAN if the distribution is empty.*/
            double GetMax() {
                UpdatePriorBin();
                double max_val = DBL_MIN;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        max_val = std::max(d.second.quantile(1), max_val);
                    }
                }
                return max_val == DBL_MIN ? NAN : max_val;
            }

            /* Get the minimum value in the distribution. Return NAN if the distribution is empty.*/
            double GetMin() {
                UpdatePriorBin();
                double min_val = DBL_MAX;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        min_val = std::min(d.second.quantile(0), min_val);
                    }
                }
                return min_val == DBL_MAX ? NAN : min_val;
            }

            /* Get the mean value in the distribution. Return NAN if the distribution is empty.*/
            double GetMean() {
                UpdatePriorBin();
                double total_val = 0;
                double total_weight = 0;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    for(auto &d: tmp.per_thread_dist) {
                        d.second.quantile(-1); //trigger tdigest to process()
                        total_val += d.second.getSum();
                        total_weight += d.second.getWeight();
                    }
                }
                return total_weight == 0 ? NAN:total_val/total_weight;
            }

            /* Return the stdDev of the values in the distribution. */
            double GetStdDev() {
                UpdatePriorBin();
                double mean = GetMean();
                double weight = GetCount();
                double variance = 0;
                for (ThreadMinBin &tmp: prior_minute_bins_) {
                    std::vector<std::vector<tdigest::Centroid>> tmp_centroids = tmp.GetCentroids();
                    for (auto &x: tmp_centroids) {
                        for (auto &c: x) {
                            variance += pow((c.mean() - mean), 2)*c.weight();
                        }
                    }
                }
                return variance == 0 ? 0 : sqrt(variance/weight);
            }

        private:
            double accuracy = 100;
            int max_bins = 10;
            uint64_t clock_milli_;
            std::deque<ThreadMinBin> prior_minute_bins_;
            std::mutex mtx_;
            ThreadMinBin cur_minute_bin_;

    };
}