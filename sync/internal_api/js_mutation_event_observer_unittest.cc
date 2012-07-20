// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/js_mutation_event_observer.h"

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "base/values.h"
#include "sync/internal_api/public/base/model_type.h"
#include "sync/internal_api/public/util/weak_handle.h"
#include "sync/js/js_event_details.h"
#include "sync/js/js_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

using ::testing::InSequence;
using ::testing::StrictMock;

class JsMutationEventObserverTest : public testing::Test {
 protected:
  JsMutationEventObserverTest() {
    js_mutation_event_observer_.SetJsEventHandler(
        mock_js_event_handler_.AsWeakHandle());
  }

 private:
  // This must be destroyed after the member variables below in order
  // for WeakHandles to be destroyed properly.
  MessageLoop message_loop_;

 protected:
  StrictMock<MockJsEventHandler> mock_js_event_handler_;
  JsMutationEventObserver js_mutation_event_observer_;

  void PumpLoop() {
    message_loop_.RunAllPending();
  }
};

TEST_F(JsMutationEventObserverTest, OnChangesApplied) {
  InSequence dummy;

  // We don't test with passwords as that requires additional setup.

  // Build a list of example ChangeRecords.
  ChangeRecord changes[MODEL_TYPE_COUNT];
  for (int i = AUTOFILL_PROFILE; i < MODEL_TYPE_COUNT; ++i) {
    changes[i].id = i;
    switch (i % 3) {
      case 0:
        changes[i].action = ChangeRecord::ACTION_ADD;
        break;
      case 1:
        changes[i].action = ChangeRecord::ACTION_UPDATE;
        break;
      default:
        changes[i].action = ChangeRecord::ACTION_DELETE;
        break;
    }
  }

  // For each i, we call OnChangesApplied() with the first arg equal
  // to i cast to ModelType and the second argument with the changes
  // starting from changes[i].

  // Set expectations for each data type.
  for (int i = AUTOFILL_PROFILE; i < MODEL_TYPE_COUNT; ++i) {
    const std::string& model_type_str =
        ModelTypeToString(ModelTypeFromInt(i));
    DictionaryValue expected_details;
    expected_details.SetString("modelType", model_type_str);
    expected_details.SetString("writeTransactionId", "0");
    ListValue* expected_changes = new ListValue();
    expected_details.Set("changes", expected_changes);
    for (int j = i; j < MODEL_TYPE_COUNT; ++j) {
      expected_changes->Append(changes[j].ToValue());
    }
    EXPECT_CALL(mock_js_event_handler_,
                HandleJsEvent("onChangesApplied",
                             HasDetailsAsDictionary(expected_details)));
  }

  // Fire OnChangesApplied() for each data type.
  for (int i = AUTOFILL_PROFILE; i < MODEL_TYPE_COUNT; ++i) {
    ChangeRecordList local_changes(changes + i, changes + arraysize(changes));
    js_mutation_event_observer_.OnChangesApplied(
        ModelTypeFromInt(i),
        0, ImmutableChangeRecordList(&local_changes));
  }

  PumpLoop();
}

TEST_F(JsMutationEventObserverTest, OnChangesComplete) {
  InSequence dummy;

  for (int i = FIRST_REAL_MODEL_TYPE; i < MODEL_TYPE_COUNT; ++i) {
    DictionaryValue expected_details;
    expected_details.SetString(
        "modelType",
        ModelTypeToString(ModelTypeFromInt(i)));
    EXPECT_CALL(mock_js_event_handler_,
                HandleJsEvent("onChangesComplete",
                             HasDetailsAsDictionary(expected_details)));
  }

  for (int i = FIRST_REAL_MODEL_TYPE; i < MODEL_TYPE_COUNT; ++i) {
    js_mutation_event_observer_.OnChangesComplete(
        ModelTypeFromInt(i));
  }
  PumpLoop();
}

}  // namespace
}  // namespace syncer
