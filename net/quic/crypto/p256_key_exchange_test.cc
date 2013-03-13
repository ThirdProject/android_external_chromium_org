// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/crypto/p256_key_exchange.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

// SharedKey just tests that the basic key exchange identity holds: that both
// parties end up with the same key.
TEST(P256KeyExchange, SharedKey) {
  for (int i = 0; i < 5; i++) {
    std::string alice_private(P256KeyExchange::NewPrivateKey());
    std::string bob_private(P256KeyExchange::NewPrivateKey());

    ASSERT_FALSE(alice_private.empty());
    ASSERT_FALSE(bob_private.empty());
    ASSERT_NE(alice_private, bob_private);

    scoped_ptr<P256KeyExchange> alice(P256KeyExchange::New(alice_private));
    scoped_ptr<P256KeyExchange> bob(P256KeyExchange::New(bob_private));

    ASSERT_TRUE(alice.get() != NULL);
    ASSERT_TRUE(bob.get() != NULL);

    const base::StringPiece alice_public(alice->public_value());
    const base::StringPiece bob_public(bob->public_value());

    std::string alice_shared, bob_shared;
    ASSERT_TRUE(alice->CalculateSharedKey(bob_public, &alice_shared));
    ASSERT_TRUE(bob->CalculateSharedKey(alice_public, &bob_shared));
    ASSERT_EQ(alice_shared, bob_shared);
  }
}

}  // namespace test
}  // namespace net

