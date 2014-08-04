// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/operations/truncate.h"

#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "chrome/browser/chromeos/file_system_provider/operations/test_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/common/extensions/api/file_system_provider.h"
#include "chrome/common/extensions/api/file_system_provider_internal.h"
#include "extensions/browser/event_router.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/async_file_util.h"

namespace chromeos {
namespace file_system_provider {
namespace operations {
namespace {

const char kExtensionId[] = "mbflcebpggnecokmikipoihdbecnjfoj";
const char kFileSystemId[] = "testing-file-system";
const int kRequestId = 2;
const base::FilePath::CharType kFilePath[] = "/kitty/and/puppy/happy";
const int64 kTruncateLength = 64;

}  // namespace

class FileSystemProviderOperationsTruncateTest : public testing::Test {
 protected:
  FileSystemProviderOperationsTruncateTest() {}
  virtual ~FileSystemProviderOperationsTruncateTest() {}

  virtual void SetUp() OVERRIDE {
    file_system_info_ =
        ProvidedFileSystemInfo(kExtensionId,
                               kFileSystemId,
                               "" /* file_system_name */,
                               true /* writable */,
                               base::FilePath() /* mount_path */);
  }

  ProvidedFileSystemInfo file_system_info_;
};

TEST_F(FileSystemProviderOperationsTruncateTest, Execute) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  Truncate truncate(NULL,
                    file_system_info_,
                    base::FilePath::FromUTF8Unsafe(kFilePath),
                    kTruncateLength,
                    base::Bind(&util::LogStatusCallback, &callback_log));
  truncate.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(truncate.Execute(kRequestId));

  ASSERT_EQ(1u, dispatcher.events().size());
  extensions::Event* event = dispatcher.events()[0];
  EXPECT_EQ(
      extensions::api::file_system_provider::OnTruncateRequested::kEventName,
      event->event_name);
  base::ListValue* event_args = event->event_args.get();
  ASSERT_EQ(1u, event_args->GetSize());

  base::DictionaryValue* options = NULL;
  ASSERT_TRUE(event_args->GetDictionary(0, &options));

  std::string event_file_system_id;
  EXPECT_TRUE(options->GetString("fileSystemId", &event_file_system_id));
  EXPECT_EQ(kFileSystemId, event_file_system_id);

  int event_request_id = -1;
  EXPECT_TRUE(options->GetInteger("requestId", &event_request_id));
  EXPECT_EQ(kRequestId, event_request_id);

  std::string event_file_path;
  EXPECT_TRUE(options->GetString("filePath", &event_file_path));
  EXPECT_EQ(kFilePath, event_file_path);

  double event_length = -1;
  EXPECT_TRUE(options->GetDouble("length", &event_length));
  EXPECT_EQ(kTruncateLength, static_cast<double>(event_length));
}

TEST_F(FileSystemProviderOperationsTruncateTest, Execute_NoListener) {
  util::LoggingDispatchEventImpl dispatcher(false /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  Truncate truncate(NULL,
                    file_system_info_,
                    base::FilePath::FromUTF8Unsafe(kFilePath),
                    kTruncateLength,
                    base::Bind(&util::LogStatusCallback, &callback_log));
  truncate.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_FALSE(truncate.Execute(kRequestId));
}

TEST_F(FileSystemProviderOperationsTruncateTest, Execute_ReadOnly) {
  util::LoggingDispatchEventImpl dispatcher(false /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  const ProvidedFileSystemInfo read_only_file_system_info(
      kExtensionId,
      kFileSystemId,
      "" /* file_system_name */,
      false /* writable */,
      base::FilePath() /* mount_path */);

  Truncate truncate(NULL,
                    file_system_info_,
                    base::FilePath::FromUTF8Unsafe(kFilePath),
                    kTruncateLength,
                    base::Bind(&util::LogStatusCallback, &callback_log));
  truncate.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_FALSE(truncate.Execute(kRequestId));
}

TEST_F(FileSystemProviderOperationsTruncateTest, OnSuccess) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  Truncate truncate(NULL,
                    file_system_info_,
                    base::FilePath::FromUTF8Unsafe(kFilePath),
                    kTruncateLength,
                    base::Bind(&util::LogStatusCallback, &callback_log));
  truncate.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(truncate.Execute(kRequestId));

  truncate.OnSuccess(kRequestId,
                     scoped_ptr<RequestValue>(new RequestValue()),
                     false /* has_more */);
  ASSERT_EQ(1u, callback_log.size());
  EXPECT_EQ(base::File::FILE_OK, callback_log[0]);
}

TEST_F(FileSystemProviderOperationsTruncateTest, OnError) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  Truncate truncate(NULL,
                    file_system_info_,
                    base::FilePath::FromUTF8Unsafe(kFilePath),
                    kTruncateLength,
                    base::Bind(&util::LogStatusCallback, &callback_log));
  truncate.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(truncate.Execute(kRequestId));

  truncate.OnError(kRequestId,
                   scoped_ptr<RequestValue>(new RequestValue()),
                   base::File::FILE_ERROR_TOO_MANY_OPENED);
  ASSERT_EQ(1u, callback_log.size());
  EXPECT_EQ(base::File::FILE_ERROR_TOO_MANY_OPENED, callback_log[0]);
}

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos
