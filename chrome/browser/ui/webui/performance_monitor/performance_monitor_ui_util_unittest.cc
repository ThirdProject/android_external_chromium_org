// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/time.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/performance_monitor/metric.h"
#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_ui_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace performance_monitor {

class PerformanceMonitorUtilTest : public ::testing::Test {

};

TEST(PerformanceMonitorUtilTest, AggregateMetricEmptyTest) {
  Database::MetricVector metric;
  const base::Time data_time = base::Time::FromDoubleT(1);
  metric.push_back(Metric(METRIC_CPU_USAGE, data_time, 1));

  const base::Time results_time = base::Time::FromDoubleT(3);
  const base::TimeDelta resolution = base::TimeDelta::FromSeconds(1);
  scoped_ptr<Database::MetricVector> aggregated_metric =
      AggregateMetric(METRIC_CPU_USAGE,
                      &metric,
                      results_time,
                      resolution,
                      AGGREGATION_METHOD_MEAN);
  ASSERT_EQ(0u, aggregated_metric->size());
  aggregated_metric = AggregateMetric(METRIC_CPU_USAGE,
                                      &metric,
                                      results_time,
                                      resolution,
                                      AGGREGATION_METHOD_MEDIAN);
  ASSERT_EQ(0u, aggregated_metric->size());
}

TEST(PerformanceMonitorUtilTest, AggregateMetricSimpleTest) {
  const base::Time data_time = base::Time::FromDoubleT(2);
  const base::Time results_time = base::Time::FromDoubleT(1);
  const base::TimeDelta results_resolution = base::TimeDelta::FromSeconds(2);

  const double value = 3.14;
  Database::MetricVector metric;
  metric.push_back(Metric(METRIC_CPU_USAGE, data_time, value));
  Database::MetricVector aggregated_metric =
      *AggregateMetric(METRIC_CPU_USAGE,
                       &metric,
                       results_time,
                       results_resolution,
                       AGGREGATION_METHOD_MEAN);

  ASSERT_EQ(1u, aggregated_metric.size());
  EXPECT_EQ(results_time + results_resolution, aggregated_metric[0].time);
  EXPECT_EQ(value, aggregated_metric[0].value);

  aggregated_metric = *AggregateMetric(METRIC_CPU_USAGE,
                                       &metric,
                                       results_time,
                                       results_resolution,
                                       AGGREGATION_METHOD_MEDIAN);
  ASSERT_EQ(1u, aggregated_metric.size());
  EXPECT_EQ(results_time + results_resolution, aggregated_metric[0].time);
  EXPECT_EQ(value, aggregated_metric[0].value);
}

TEST(PerformanceMonitorUtilTest, AggregateMetricDenseTest) {
  base::Time current_data_time = base::Time::FromDoubleT(2);
  const base::TimeDelta data_resolution = base::TimeDelta::FromSeconds(1);
  const base::Time results_time = base::Time::FromDoubleT(6);
  const base::TimeDelta results_resolution = base::TimeDelta::FromSeconds(4);
  double current_value = 1;
  int num_points = 12;
  Database::MetricVector metric;

  for (int i = 0; i < num_points; ++i) {
    metric.push_back(Metric(METRIC_CPU_USAGE,
                            current_data_time,
                            current_value));
    current_value *= 2;
    current_data_time += data_resolution;
  }
  Database::MetricVector aggregated_metric =
      *AggregateMetric(METRIC_CPU_USAGE,
                       &metric,
                       results_time,
                       results_resolution,
                       AGGREGATION_METHOD_MEAN);
  // The first 4 points get ignored because they are before the start time.
  // The remaining 8 points are aggregated into two data points.
  ASSERT_EQ(2u, aggregated_metric.size());
  EXPECT_EQ(results_time + results_resolution, aggregated_metric[0].time);
  EXPECT_DOUBLE_EQ((32 + 64 + 128 + 256) / 4.0, aggregated_metric[0].value);
  EXPECT_EQ(results_time + (2 * results_resolution),
            aggregated_metric[1].time);
  // Since we don't have data for the time of 14, we stretch out the 2048.
  EXPECT_DOUBLE_EQ((512 + 1024 + 2048 + 2048) / 4.0,
                   aggregated_metric[1].value);

  aggregated_metric = *AggregateMetric(METRIC_CPU_USAGE,
                                       &metric,
                                       results_time,
                                       results_resolution,
                                       AGGREGATION_METHOD_MEDIAN);
  ASSERT_EQ(2u, aggregated_metric.size());
  EXPECT_EQ(results_time + results_resolution, aggregated_metric[0].time);
  EXPECT_EQ(results_time + 2 * results_resolution,
            aggregated_metric[1].time);
  EXPECT_EQ(48, aggregated_metric[0].value);
  EXPECT_EQ(768, aggregated_metric[1].value);
}

TEST(PerformanceMonitorUtilTest, AggregateMetricSparseTest) {
  Database::MetricVector metric;

  const base::Time data_time1 = base::Time::FromDoubleT(20);
  const double value1 = 3.14;
  metric.push_back(Metric(METRIC_CPU_USAGE, data_time1, value1));
  const base::Time data_time2 = base::Time::FromDoubleT(40);
  const double value2 = 6.28;
  metric.push_back(Metric(METRIC_CPU_USAGE, data_time2, value2));
  const base::Time data_time3 = base::Time::FromDoubleT(60);
  const double value3 = 9.42;
  metric.push_back(Metric(METRIC_CPU_USAGE, data_time3, value3));

  const base::Time results_time = base::Time::FromDoubleT(19);
  const base::TimeDelta results_resolution = base::TimeDelta::FromSeconds(2);
  Database::MetricVector aggregated_metric =
      *AggregateMetric(METRIC_CPU_USAGE,
                       &metric,
                       results_time,
                       results_resolution,
                       AGGREGATION_METHOD_MEAN);

  // The first aggregation point is split between the first value and the second
  // value. The second is split between the second and third. The third doesn't
  // have any data after it so the aggregation is the same value.
  ASSERT_EQ(3u, aggregated_metric.size());
  EXPECT_EQ(results_time + 1 * results_resolution,
            aggregated_metric[0].time);
  EXPECT_EQ((value1 + value2) / 2, aggregated_metric[0].value);
  EXPECT_EQ(results_time + 11 * results_resolution,
            aggregated_metric[1].time);
  EXPECT_EQ((value2 + value3) / 2, aggregated_metric[1].value);
  EXPECT_EQ(results_time + 21 * results_resolution,
            aggregated_metric[2].time);
  EXPECT_EQ(value3, aggregated_metric[2].value);

  // For median values, we go from [start, end). Thus, since each of these are
  // one window apart, each value will have it's own window.
  aggregated_metric = *AggregateMetric(METRIC_CPU_USAGE,
                                       &metric,
                                       results_time,
                                       results_resolution,
                                       AGGREGATION_METHOD_MEDIAN);
  ASSERT_EQ(3u, aggregated_metric.size());
  EXPECT_EQ(results_time + 1 * results_resolution,
            aggregated_metric[0].time);
  EXPECT_EQ(value1, aggregated_metric[0].value);
  EXPECT_EQ(results_time + 11 * results_resolution,
            aggregated_metric[1].time);
  EXPECT_EQ(value2, aggregated_metric[1].value);
  EXPECT_EQ(results_time + 21 * results_resolution,
            aggregated_metric[2].time);
  EXPECT_EQ(value3, aggregated_metric[2].value);
}

}  // namespace performance_monitor
