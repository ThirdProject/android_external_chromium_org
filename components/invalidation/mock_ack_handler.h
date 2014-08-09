// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_MOCK_ACK_HANDLER_H_
#define COMPONENTS_INVALIDATION_MOCK_ACK_HANDLER_H_

#include <map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/invalidation/ack_handler.h"
#include "components/invalidation/invalidation_export.h"
#include "components/invalidation/invalidation_util.h"

namespace syncer {

class Invalidation;

// This AckHandler implementation colaborates with the FakeInvalidationService
// to enable unit tests to assert that invalidations are being acked properly.
class INVALIDATION_EXPORT MockAckHandler
  : public AckHandler,
    public base::SupportsWeakPtr<MockAckHandler> {
 public:
  MockAckHandler();
  virtual ~MockAckHandler();

  // Sets up some internal state to track this invalidation, and modifies it so
  // that its Acknowledge() and Drop() methods will route back to us.
  void RegisterInvalidation(Invalidation* invalidation);

  // No one was listening for this invalidation, so no one will receive it or
  // ack it.  We keep track of it anyway to let tests make assertions about it.
  void RegisterUnsentInvalidation(Invalidation* invalidation);

  // Returns true if the specified invalidaition has been delivered, but has not
  // been acknowledged yet.
  bool IsUnacked(const Invalidation& invalidation) const;

  // Returns true if the specified invalidation has been delivered and
  // acknowledged.
  bool IsAcknowledged(const Invalidation& invalidation) const;

  // Returns true if the specified invalidation has been delivered and
  // dropped.
  bool IsDropped(const Invalidation& invalidation) const;

  // Returns true if the specified invalidation was never delivered.
  bool IsUnsent(const Invalidation& invalidation) const;

  // Retruns true if all invalidations have been acked and all drops recovered.
  bool AllInvalidationsAccountedFor() const;

  // Implementation of AckHandler.
  virtual void Acknowledge(
      const invalidation::ObjectId& id,
      const AckHandle& handle) OVERRIDE;
  virtual void Drop(
      const invalidation::ObjectId& id,
      const AckHandle& handle) OVERRIDE;

 private:
  typedef std::vector<syncer::Invalidation> InvalidationVector;
  typedef std::map<invalidation::ObjectId,
                   AckHandle,
                   ObjectIdLessThan> IdHandleMap;

  InvalidationVector unsent_invalidations_;
  InvalidationVector unacked_invalidations_;
  InvalidationVector acked_invalidations_;
  InvalidationVector dropped_invalidations_;

  IdHandleMap unrecovered_drop_events_;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_MOCK_ACK_HANDLER_H_
