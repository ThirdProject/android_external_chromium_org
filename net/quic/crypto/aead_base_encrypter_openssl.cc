// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/crypto/aead_base_encrypter.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <string.h>

#include "base/memory/scoped_ptr.h"

using base::StringPiece;

namespace net {

namespace {

// Clear OpenSSL error stack.
void ClearOpenSslErrors() {
#ifdef NDEBUG
  while (ERR_get_error()) {}
#else
  while (unsigned long error = ERR_get_error()) {
    char buf[120];
    ERR_error_string_n(error, buf, arraysize(buf));
    DLOG(ERROR) << "OpenSSL error: " << buf;
  }
#endif
}

}  // namespace

AeadBaseEncrypter::AeadBaseEncrypter(const EVP_AEAD* aead_alg,
                                     size_t key_size,
                                     size_t auth_tag_size,
                                     size_t nonce_prefix_size)
    : aead_alg_(aead_alg),
      key_size_(key_size),
      auth_tag_size_(auth_tag_size),
      nonce_prefix_size_(nonce_prefix_size) {
  DCHECK_LE(key_size_, sizeof(key_));
  DCHECK_LE(nonce_prefix_size_, sizeof(nonce_prefix_));
}

AeadBaseEncrypter::~AeadBaseEncrypter() {}

bool AeadBaseEncrypter::SetKey(StringPiece key) {
  DCHECK_EQ(key.size(), key_size_);
  if (key.size() != key_size_) {
    return false;
  }
  memcpy(key_, key.data(), key.size());

  EVP_AEAD_CTX_cleanup(ctx_.get());

  if (!EVP_AEAD_CTX_init(ctx_.get(), aead_alg_, key_, key_size_,
                         auth_tag_size_, NULL)) {
    ClearOpenSslErrors();
    return false;
  }

  return true;
}

bool AeadBaseEncrypter::SetNoncePrefix(StringPiece nonce_prefix) {
  DCHECK_EQ(nonce_prefix.size(), nonce_prefix_size_);
  if (nonce_prefix.size() != nonce_prefix_size_) {
    return false;
  }
  memcpy(nonce_prefix_, nonce_prefix.data(), nonce_prefix.size());
  return true;
}

bool AeadBaseEncrypter::Encrypt(StringPiece nonce,
                                StringPiece associated_data,
                                StringPiece plaintext,
                                unsigned char* output) {
  if (nonce.size() != nonce_prefix_size_ + sizeof(QuicPacketSequenceNumber)) {
    return false;
  }

  ssize_t len = EVP_AEAD_CTX_seal(
      ctx_.get(), output, plaintext.size() + auth_tag_size_,
      reinterpret_cast<const uint8_t*>(nonce.data()), nonce.size(),
      reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size(),
      reinterpret_cast<const uint8_t*>(associated_data.data()),
      associated_data.size());

  if (len < 0) {
    ClearOpenSslErrors();
    return false;
  }

  return true;
}

QuicData* AeadBaseEncrypter::EncryptPacket(
    QuicPacketSequenceNumber sequence_number,
    StringPiece associated_data,
    StringPiece plaintext) {
  size_t ciphertext_size = GetCiphertextSize(plaintext.length());
  scoped_ptr<char[]> ciphertext(new char[ciphertext_size]);

  // TODO(ianswett): Introduce a check to ensure that we don't encrypt with the
  // same sequence number twice.
  uint8 nonce[sizeof(nonce_prefix_) + sizeof(sequence_number)];
  const size_t nonce_size = nonce_prefix_size_ + sizeof(sequence_number);
  DCHECK_LE(nonce_size, sizeof(nonce));
  memcpy(nonce, nonce_prefix_, nonce_prefix_size_);
  memcpy(nonce + nonce_prefix_size_, &sequence_number, sizeof(sequence_number));
  if (!Encrypt(StringPiece(reinterpret_cast<char*>(nonce), nonce_size),
               associated_data, plaintext,
               reinterpret_cast<unsigned char*>(ciphertext.get()))) {
    return NULL;
  }

  return new QuicData(ciphertext.release(), ciphertext_size, true);
}

size_t AeadBaseEncrypter::GetKeySize() const { return key_size_; }

size_t AeadBaseEncrypter::GetNoncePrefixSize() const {
  return nonce_prefix_size_;
}

size_t AeadBaseEncrypter::GetMaxPlaintextSize(size_t ciphertext_size) const {
  return ciphertext_size - auth_tag_size_;
}

size_t AeadBaseEncrypter::GetCiphertextSize(size_t plaintext_size) const {
  return plaintext_size + auth_tag_size_;
}

StringPiece AeadBaseEncrypter::GetKey() const {
  return StringPiece(reinterpret_cast<const char*>(key_), key_size_);
}

StringPiece AeadBaseEncrypter::GetNoncePrefix() const {
  if (nonce_prefix_size_ == 0) {
    return StringPiece();
  }
  return StringPiece(reinterpret_cast<const char*>(nonce_prefix_),
                     nonce_prefix_size_);
}

}  // namespace net
