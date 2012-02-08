// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_SHARED_CHANGE_PROCESSOR_MOCK_H_
#define CHROME_BROWSER_SYNC_GLUE_SHARED_CHANGE_PROCESSOR_MOCK_H_
#pragma once

#include "chrome/browser/sync/api/sync_change.h"
#include "chrome/browser/sync/glue/shared_change_processor.h"
#include "chrome/browser/sync/internal_api/includes/unrecoverable_error_handler.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace browser_sync {

class SharedChangeProcessorMock : public SharedChangeProcessor {
 public:
  SharedChangeProcessorMock();

  MOCK_METHOD4(Connect, bool(
      ProfileSyncComponentsFactory*,
      ProfileSyncService*,
      UnrecoverableErrorHandler*,
      const base::WeakPtr<SyncableService>&));
  MOCK_METHOD0(Disconnect, bool());
  MOCK_METHOD2(ProcessSyncChanges,
               SyncError(const tracked_objects::Location&,
                         const SyncChangeList&));
  MOCK_METHOD2(GetSyncDataForType,
               SyncError(syncable::ModelType,
                         SyncDataList*));
  MOCK_METHOD2(SyncModelHasUserCreatedNodes,
               bool(syncable::ModelType,
                    bool*));
  MOCK_METHOD1(CryptoReadyIfNecessary, bool(syncable::ModelType));
  MOCK_METHOD3(ActivateDataType,
               void(ProfileSyncService*, syncable::ModelType,
                    browser_sync::ModelSafeGroup));

 protected:
  virtual ~SharedChangeProcessorMock();
  MOCK_METHOD2(OnUnrecoverableError, void(const tracked_objects::Location&,
                                          const std::string&));

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedChangeProcessorMock);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_SHARED_CHANGE_PROCESSOR_MOCK_H_
