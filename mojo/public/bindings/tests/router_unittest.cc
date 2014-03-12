// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>
#include <string.h>

#include "mojo/public/bindings/lib/message_builder.h"
#include "mojo/public/bindings/lib/message_queue.h"
#include "mojo/public/bindings/lib/router.h"
#include "mojo/public/environment/environment.h"
#include "mojo/public/system/macros.h"
#include "mojo/public/utility/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

void AllocRequestMessage(uint32_t name, const char* text, Message* message) {
  size_t payload_size = strlen(text) + 1;  // Plus null terminator.
  internal::RequestMessageBuilder builder(name, payload_size);
  memcpy(builder.buffer()->Allocate(payload_size), text, payload_size);
  builder.Finish(message);
}

void AllocResponseMessage(uint32_t name, const char* text,
                          uint64_t request_id, Message* message) {
  size_t payload_size = strlen(text) + 1;  // Plus null terminator.
  internal::ResponseMessageBuilder builder(name, payload_size, request_id);
  memcpy(builder.buffer()->Allocate(payload_size), text, payload_size);
  builder.Finish(message);
}

class MessageAccumulator : public MessageReceiver {
 public:
  MessageAccumulator() {
  }

  virtual bool Accept(Message* message) MOJO_OVERRIDE {
    queue_.Push(message);
    return true;
  }

  virtual bool AcceptWithResponder(Message* message, MessageReceiver* responder)
      MOJO_OVERRIDE {
    return false;
  }

  bool IsEmpty() const {
    return queue_.IsEmpty();
  }

  void Pop(Message* message) {
    queue_.Pop(message);
  }

 private:
  internal::MessageQueue queue_;
};

class ResponseGenerator : public MessageReceiver {
 public:
  ResponseGenerator() {
  }

  virtual bool Accept(Message* message) MOJO_OVERRIDE {
    return false;
  }

  virtual bool AcceptWithResponder(Message* message, MessageReceiver* responder)
      MOJO_OVERRIDE {
    EXPECT_TRUE(message->has_flag(internal::kMessageExpectsResponse));

    return SendResponse(message->name(), message->request_id(), responder);
  }

  bool SendResponse(uint32_t name, uint64_t request_id,
                    MessageReceiver* responder) {
    Message response;
    AllocResponseMessage(name, "world", request_id, &response);
    return responder->Accept(&response);
  }
};

class LazyResponseGenerator : public ResponseGenerator {
 public:
  virtual bool AcceptWithResponder(Message* message, MessageReceiver* responder)
      MOJO_OVERRIDE {
    name_ = message->name();
    request_id_ = message->request_id();
    responder_ = responder;
    return true;
  }

  bool has_responder() const { return !!responder_; }

  void Complete() {
    SendResponse(name_, request_id_, responder_);
    responder_ = NULL;
  }

 private:
  MessageReceiver* responder_;
  uint32_t name_;
  uint32_t request_id_;
};

class RouterTest : public testing::Test {
 public:
  RouterTest() {
  }

  virtual void SetUp() MOJO_OVERRIDE {
    CreateMessagePipe(&handle0_, &handle1_);
  }

  virtual void TearDown() MOJO_OVERRIDE {
  }

  void PumpMessages() {
    loop_.RunUntilIdle();
  }

 protected:
  ScopedMessagePipeHandle handle0_;
  ScopedMessagePipeHandle handle1_;

 private:
  Environment env_;
  RunLoop loop_;
};

TEST_F(RouterTest, BasicRequestResponse) {
  internal::Router router0(handle0_.Pass());
  internal::Router router1(handle1_.Pass());

  ResponseGenerator generator;
  router1.set_incoming_receiver(&generator);

  Message request;
  AllocRequestMessage(1, "hello", &request);

  MessageAccumulator accumulator;
  router0.AcceptWithResponder(&request, &accumulator);

  PumpMessages();

  EXPECT_FALSE(accumulator.IsEmpty());

  Message response;
  accumulator.Pop(&response);

  EXPECT_EQ(std::string("world"),
            std::string(reinterpret_cast<const char*>(response.payload())));
}

TEST_F(RouterTest, RequestWithNoReceiver) {
  internal::Router router0(handle0_.Pass());
  internal::Router router1(handle1_.Pass());

  // Without an incoming receiver set on router1, we expect router0 to observe
  // an error as a result of sending a message.

  Message request;
  AllocRequestMessage(1, "hello", &request);

  MessageAccumulator accumulator;
  router0.AcceptWithResponder(&request, &accumulator);

  PumpMessages();

  EXPECT_TRUE(router0.encountered_error());
  EXPECT_TRUE(router1.encountered_error());
  EXPECT_TRUE(accumulator.IsEmpty());
}

TEST_F(RouterTest, LateResponse) {
  // Test that things won't blow up if we try to send a message to a
  // MessageReceiver, which was given to us via AcceptWithResponder,
  // after the router has gone away.

  LazyResponseGenerator generator;
  {
    internal::Router router0(handle0_.Pass());
    internal::Router router1(handle1_.Pass());

    router1.set_incoming_receiver(&generator);

    Message request;
    AllocRequestMessage(1, "hello", &request);

    MessageAccumulator accumulator;
    router0.AcceptWithResponder(&request, &accumulator);

    PumpMessages();

    EXPECT_TRUE(generator.has_responder());

  }

  generator.Complete();  // This should end up doing nothing.
}

}  // namespace
}  // namespace test
}  // namespace mojo
