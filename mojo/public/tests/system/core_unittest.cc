// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C API.

#include "mojo/public/system/core.h"

#include <string.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(CoreTest, GetTimeTicksNow) {
  const MojoTimeTicks start = MojoGetTimeTicksNow();
  EXPECT_NE(static_cast<MojoTimeTicks>(0), start)
      << "MojoGetTimeTicksNow should return nonzero value";
}

// The only handle that's guaranteed to be invalid is |MOJO_HANDLE_INVALID|.
// Tests that everything that takes a handle properly recognizes it.
TEST(CoreTest, InvalidHandle) {
  MojoHandle h0, h1;
  MojoWaitFlags wf;
  char buffer[10] = { 0 };
  uint32_t buffer_size;
  void* write_pointer;
  const void* read_pointer;

  // Close:
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(MOJO_HANDLE_INVALID));

  // Wait:
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWait(MOJO_HANDLE_INVALID, MOJO_WAIT_FLAG_EVERYTHING, 1000000));
  h0 = MOJO_HANDLE_INVALID;
  wf = MOJO_WAIT_FLAG_EVERYTHING;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWaitMany(&h0, &wf, 1, MOJO_DEADLINE_INDEFINITE));

  // Message pipe:
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWriteMessage(h0, buffer, 3, NULL, 0,
                             MOJO_WRITE_MESSAGE_FLAG_NONE));
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoReadMessage(h0, buffer, &buffer_size, NULL, NULL,
                            MOJO_READ_MESSAGE_FLAG_NONE));

  // Data pipe:
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWriteData(h0, buffer, &buffer_size, MOJO_WRITE_DATA_FLAG_NONE));
  write_pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoBeginWriteData(h0, &write_pointer, &buffer_size,
                               MOJO_WRITE_DATA_FLAG_NONE));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoEndWriteData(h0, 1));
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoReadData(h0, buffer, &buffer_size, MOJO_READ_DATA_FLAG_NONE));
  read_pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoBeginReadData(h0, &read_pointer, &buffer_size,
                              MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoEndReadData(h0, 1));

  // Shared buffer:
  h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoDuplicateBufferHandle(h0, NULL, &h1));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoMapBuffer(h0, 0, 1, &write_pointer, MOJO_MAP_BUFFER_FLAG_NONE));
}

TEST(CoreTest, BasicMessagePipe) {
  MojoHandle h0, h1;
  MojoWaitFlags wf;
  char buffer[10] = { 0 };
  uint32_t buffer_size;

  h0 = MOJO_HANDLE_INVALID;
  h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(&h0, &h1));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);

  // Shouldn't be readable.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWait(h0, MOJO_WAIT_FLAG_READABLE, 0));

  // Should be writable.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(h0, MOJO_WAIT_FLAG_WRITABLE, 0));

  // Try to read.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoReadMessage(h0, buffer, &buffer_size, NULL, NULL,
                            MOJO_READ_MESSAGE_FLAG_NONE));

  // Write to |h1|.
  static const char kHello[] = "hello";
  buffer_size = static_cast<uint32_t>(sizeof(kHello));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteMessage(h1, kHello, buffer_size, NULL, 0,
                             MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |h0| should be readable.
  wf = MOJO_WAIT_FLAG_READABLE;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitMany(&h0, &wf, 1, MOJO_DEADLINE_INDEFINITE));

  // Read from |h0|.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadMessage(h0, buffer, &buffer_size, NULL, NULL,
                            MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_EQ(static_cast<uint32_t>(sizeof(kHello)), buffer_size);
  EXPECT_STREQ(kHello, buffer);

  // |h0| should no longer be readable.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWait(h0, MOJO_WAIT_FLAG_READABLE, 10));

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));

  // |h1| should no longer be readable or writable.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoWait(h1, MOJO_WAIT_FLAG_READABLE | MOJO_WAIT_FLAG_WRITABLE,
                     1000));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
}

TEST(CoreTest, BasicDataPipe) {
  MojoHandle hp, hc;
  MojoWaitFlags wf;
  char buffer[20] = { 0 };
  uint32_t buffer_size;
  void* write_pointer;
  const void* read_pointer;

  hp = MOJO_HANDLE_INVALID;
  hc = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateDataPipe(NULL, &hp, &hc));
  EXPECT_NE(hp, MOJO_HANDLE_INVALID);
  EXPECT_NE(hc, MOJO_HANDLE_INVALID);

  // The consumer |hc| shouldn't be readable.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWait(hc, MOJO_WAIT_FLAG_READABLE, 0));

  // The producer |hp| should be writable.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hp, MOJO_WAIT_FLAG_WRITABLE, 0));

  // Try to read from |hc|.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoReadData(hc, buffer, &buffer_size, MOJO_READ_DATA_FLAG_NONE));

  // Try to begin a two-phase read from |hc|.
  read_pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoBeginReadData(hc, &read_pointer, &buffer_size,
                              MOJO_READ_DATA_FLAG_NONE));

  // Write to |hp|.
  static const char kHello[] = "hello ";
  // Don't include terminating null.
  buffer_size = static_cast<uint32_t>(strlen(kHello));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWriteData(hp, kHello, &buffer_size,
                          MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |hc| should be(come) readable.
  wf = MOJO_WAIT_FLAG_READABLE;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWaitMany(&hc, &wf, 1, MOJO_DEADLINE_INDEFINITE));

  // Do a two-phase write to |hp|.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoBeginWriteData(hp, &write_pointer, &buffer_size,
                               MOJO_WRITE_DATA_FLAG_NONE));
  static const char kWorld[] = "world";
  ASSERT_GE(buffer_size, sizeof(kWorld));
  // Include the terminating null.
  memcpy(write_pointer, kWorld, sizeof(kWorld));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoEndWriteData(hp, static_cast<uint32_t>(sizeof(kWorld))));

  // Read one character from |hc|.
  memset(buffer, 0, sizeof(buffer));
  buffer_size = 1;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadData(hc, buffer, &buffer_size, MOJO_READ_DATA_FLAG_NONE));

  // Close |hp|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hp));

  // |hc| should still be readable.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWait(hc, MOJO_WAIT_FLAG_READABLE, 0));

  // Do a two-phase read from |hc|.
  read_pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoBeginReadData(hc, &read_pointer, &buffer_size,
                              MOJO_READ_DATA_FLAG_NONE));
  ASSERT_LE(buffer_size, sizeof(buffer) - 1);
  memcpy(&buffer[1], read_pointer, buffer_size);
  EXPECT_EQ(MOJO_RESULT_OK, MojoEndReadData(hc, buffer_size));
  EXPECT_STREQ("hello world", buffer);

  // |hc| should no longer be readable.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoWait(hc, MOJO_WAIT_FLAG_READABLE, 1000));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(hc));

  // TODO(vtl): Test the other way around -- closing the consumer should make
  // the producer never-writable?
}

TEST(CoreTest, BasicSharedBuffer) {
  MojoHandle h0, h1;
  void* pointer;

  // Create a shared buffer (|h0|).
  h0 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateSharedBuffer(NULL, 100, &h0));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);

  // Map everything.
  pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoMapBuffer(h0, 0, 100, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);
  static_cast<char*>(pointer)[50] = 'x';

  // Duplicate |h0| to |h1|.
  h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoDuplicateBufferHandle(h0, NULL, &h1));
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));

  // The mapping should still be good.
  static_cast<char*>(pointer)[51] = 'y';

  // Unmap it.
  // TODO(vtl): Not yet implemented.
  EXPECT_EQ(MOJO_RESULT_UNIMPLEMENTED, MojoUnmapBuffer(pointer));

  // Map half of |h1|.
  pointer = NULL;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoMapBuffer(h1, 50, 50, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);

  // It should have what we wrote.
  EXPECT_EQ('x', static_cast<char*>(pointer)[0]);
  EXPECT_EQ('y', static_cast<char*>(pointer)[1]);

  // Unmap it.
  // TODO(vtl): Not yet implemented.
  EXPECT_EQ(MOJO_RESULT_UNIMPLEMENTED, MojoUnmapBuffer(pointer));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
}

// Defined in core_unittest_pure_c.c.
extern "C" const char* MinimalCTest(void);

// This checks that things actually work in C (not C++).
TEST(CoreTest, MinimalCTest) {
  const char* failure = MinimalCTest();
  EXPECT_TRUE(failure == NULL) << failure;
}

// TODO(vtl): Add multi-threaded tests.

}  // namespace
}  // namespace mojo
