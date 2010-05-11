// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop.h"
#include "base/path_service.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/extensions/image_loading_tracker.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_resource.h"
#include "chrome/common/json_value_serializer.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "gfx/size.h"
#include "testing/gtest/include/gtest/gtest.h"

class ImageLoadingTrackerTest : public testing::Test,
                                public ImageLoadingTracker::Observer {
 public:
  ImageLoadingTrackerTest()
      : image_loaded_count_(0),
        quit_in_image_loaded_(false),
        ui_thread_(ChromeThread::UI, &ui_loop_),
        file_thread_(ChromeThread::FILE),
        io_thread_(ChromeThread::IO) {
  }

  virtual void OnImageLoaded(SkBitmap* image, ExtensionResource resource,
                             int index) {
    image_loaded_count_++;
    if (quit_in_image_loaded_)
      MessageLoop::current()->Quit();
    if (image)
      image_ = *image;
    else
      image_.reset();
  }

  void WaitForImageLoad() {
    quit_in_image_loaded_ = true;
    MessageLoop::current()->Run();
    quit_in_image_loaded_ = false;
  }

  int image_loaded_count() {
    int result = image_loaded_count_;
    image_loaded_count_ = 0;
    return result;
  }

  Extension* CreateExtension() {
    // Create and load an extension.
    FilePath test_file;
    if (!PathService::Get(chrome::DIR_TEST_DATA, &test_file)) {
      EXPECT_FALSE(true);
      return NULL;
    }
    test_file = test_file.AppendASCII("extensions")
                         .AppendASCII("image_loading_tracker");
    int error_code = 0;
    std::string error;
    JSONFileValueSerializer serializer(test_file.AppendASCII("app.json"));
    scoped_ptr<DictionaryValue> valid_value(
        static_cast<DictionaryValue*>(serializer.Deserialize(&error_code,
                                                             &error)));
    EXPECT_EQ(0, error_code) << error;
    if (error_code != 0)
      return NULL;

    EXPECT_TRUE(valid_value.get());
    if (!valid_value.get())
      return NULL;

    scoped_ptr<Extension> extension(new Extension(test_file));
    if (!extension->InitFromValue(*valid_value, false, &error))
      return NULL;

    return extension.release();
  }

  SkBitmap image_;

 private:
  virtual void SetUp() {
    file_thread_.Start();
    io_thread_.Start();
  }

  int image_loaded_count_;
  bool quit_in_image_loaded_;
  MessageLoop ui_loop_;
  ChromeThread ui_thread_;
  ChromeThread file_thread_;
  ChromeThread io_thread_;
};

// Tests asking ImageLoadingTracker to cache pushes the result to the Extension.
TEST_F(ImageLoadingTrackerTest, Cache) {
  scoped_ptr<Extension> extension(CreateExtension());
  ASSERT_TRUE(extension.get() != NULL);

  ExtensionResource image_resource =
      extension->GetIconPath(Extension::EXTENSION_ICON_SMALLISH);
  ImageLoadingTracker loader(static_cast<ImageLoadingTracker::Observer*>(this));
  loader.LoadImage(extension.get(),
                   image_resource,
                   gfx::Size(Extension::EXTENSION_ICON_SMALLISH,
                             Extension::EXTENSION_ICON_SMALLISH),
                   ImageLoadingTracker::CACHE);

  // The image isn't cached, so we should not have received notification.
  EXPECT_EQ(0, image_loaded_count());

  WaitForImageLoad();

  // We should have gotten the image.
  EXPECT_EQ(1, image_loaded_count());

  // Check that the image was loaded.
  EXPECT_EQ(Extension::EXTENSION_ICON_SMALLISH, image_.width());

  // The image should be cached in the Extension.
  EXPECT_TRUE(extension->HasCachedImage(image_resource));

  // Make sure the image is in the extension.
  EXPECT_EQ(Extension::EXTENSION_ICON_SMALLISH,
            extension->GetCachedImage(image_resource).width());

  // Ask the tracker for the image again, this should call us back immediately.
  loader.LoadImage(extension.get(),
                   image_resource,
                   gfx::Size(Extension::EXTENSION_ICON_SMALLISH,
                             Extension::EXTENSION_ICON_SMALLISH),
                   ImageLoadingTracker::CACHE);
  // We should have gotten the image.
  EXPECT_EQ(1, image_loaded_count());

  // Check that the image was loaded.
  EXPECT_EQ(Extension::EXTENSION_ICON_SMALLISH, image_.width());
}

// Tests deleting an extension while waiting for the image to load doesn't cause
// problems.
TEST_F(ImageLoadingTrackerTest, DeleteExtensionWhileWaitingForCache) {
  scoped_ptr<Extension> extension(CreateExtension());
  ASSERT_TRUE(extension.get() != NULL);

  ExtensionResource image_resource =
      extension->GetIconPath(Extension::EXTENSION_ICON_SMALLISH);
  ImageLoadingTracker loader(static_cast<ImageLoadingTracker::Observer*>(this));
  loader.LoadImage(extension.get(),
                   image_resource,
                   gfx::Size(Extension::EXTENSION_ICON_SMALLISH,
                             Extension::EXTENSION_ICON_SMALLISH),
                   ImageLoadingTracker::CACHE);

  // The image isn't cached, so we should not have received notification.
  EXPECT_EQ(0, image_loaded_count());

  // Send out notification the extension was uninstalled.
  NotificationService::current()->Notify(
      NotificationType::EXTENSION_UNLOADED,
      NotificationService::AllSources(),
      Details<Extension>(extension.get()));

  // Chuck the extension, that way if anyone tries to access it we should crash
  // or get valgrind errors.
  extension.reset();

  WaitForImageLoad();

  // Even though we deleted the extension, we should still get the image.
  // We should still have gotten the image.
  EXPECT_EQ(1, image_loaded_count());

  // Check that the image was loaded.
  EXPECT_EQ(Extension::EXTENSION_ICON_SMALLISH, image_.width());
}
