// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_METRICS_METRICS_SERVICE_BASE_H_
#define CHROME_COMMON_METRICS_METRICS_SERVICE_BASE_H_

#include "base/basictypes.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_flattener.h"
#include "base/metrics/histogram_snapshot_manager.h"
#include "chrome/common/metrics/metrics_log_manager.h"

// This class provides base functionality for logging metrics data.
// TODO(ananta): Factor out more common code from chrome and chrome frame
// metrics service into this class.
class MetricsServiceBase : public base::HistogramFlattener {
 public:
  // HistogramFlattener interface (override) methods.
  virtual void RecordDelta(const base::Histogram& histogram,
                           const base::Histogram::SampleSet& snapshot) OVERRIDE;
  virtual void InconsistencyDetected(int problem) OVERRIDE;
  virtual void UniqueInconsistencyDetected(int problem) OVERRIDE;
  virtual void SnapshotProblemResolved(int amount) OVERRIDE;

 protected:
  MetricsServiceBase();
  virtual ~MetricsServiceBase();

  // The metrics servers' URLs, for XML and protobuf uploads.
  static const char kServerUrlXml[];
  static const char kServerUrlProto[];

  // The MIME types for the uploaded metrics data, for XML and protobuf uploads.
  static const char kMimeTypeXml[];
  static const char kMimeTypeProto[];

  // Record complete list of histograms into the current log.
  // Called when we close a log.
  void RecordCurrentHistograms();

  // Manager for the various in-flight logs.
  MetricsLogManager log_manager_;

 private:
  // |histogram_snapshot_manager_| prepares histogram deltas for transmission.
  base::HistogramSnapshotManager histogram_snapshot_manager_;

  DISALLOW_COPY_AND_ASSIGN(MetricsServiceBase);
};

#endif  // CHROME_COMMON_METRICS_METRICS_SERVICE_BASE_H_
