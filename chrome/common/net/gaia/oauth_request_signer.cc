// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/net/gaia/oauth_request_signer.h"

#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <ctime>
#include <map>
#include <string>

#include "base/base64.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/stringprintf.h"
#include "base/time.h"
#include "crypto/hmac.h"
#include "googleurl/src/gurl.h"

namespace {

static const int kHexBase = 16;
static char kHexDigits[] = "0123456789ABCDEF";
static const size_t kHmacDigestLength = 20;
static const int kMaxNonceLength = 30;
static const int kMinNonceLength = 15;

static const char kOAuthConsumerKeyLabel[] = "oauth_consumer_key";
static const char kOAuthConsumerSecretLabel[] = "oauth_consumer_secret";
static const char kOAuthNonceCharacters[] =
    "abcdefghijklmnopqrstuvwyz"
    "ABCDEFGHIJKLMNOPQRSTUVWYZ"
    "0123456789_";
static const char kOAuthNonceLabel[] = "oauth_nonce";
static const char kOAuthSignatureLabel[] = "oauth_signature";
static const char kOAuthSignatureMethodLabel[] = "oauth_signature_method";
static const char kOAuthTimestampLabel[] = "oauth_timestamp";
static const char kOAuthTokenLabel[] = "oauth_token";
static const char kOAuthTokenSecretLabel[] = "oauth_token_secret";
static const char kOAuthVersion[] = "1.0";
static const char kOAuthVersionLabel[] = "oauth_version";

enum ParseQueryState {
  START_STATE,
  KEYWORD_STATE,
  VALUE_STATE,
};

const std::string HttpMethodName(OAuthRequestSigner::HttpMethod method) {
  switch (method) {
    case OAuthRequestSigner::GET_METHOD:
      return "GET";
    case OAuthRequestSigner::POST_METHOD:
      return "POST";
  }
  NOTREACHED();
  return *(new std::string());
}

const std::string SignatureMethodName(
    OAuthRequestSigner::SignatureMethod method) {
  switch (method) {
    case OAuthRequestSigner::HMAC_SHA1_SIGNATURE:
      return "HMAC-SHA1";
    case OAuthRequestSigner::RSA_SHA1_SIGNATURE:
      return "RSA-SHA1";
    case OAuthRequestSigner::PLAINTEXT_SIGNATURE:
      return "PLAINTEXT";
  }
  NOTREACHED();
  return *(new std::string());
}

// The form of percent encoding used for OAuth request signing is very
// specific and strict.  See http://oauth.net/core/1.0/#encoding_parameters.
//
// Any character which is in the "unreserved set" must not be encoded.
// All other characters must be encoded.
//
// The unreserved set is comprised of the alphanumeric characters and these
// others:
//   - minus (-)
//   - period (.)
//   - underscore (_)
//   - tilde (~)
std::string EncodedOAuthParameter(const std::string& text) {
  std::string result = "";
  std::string::const_iterator cursor;
  std::string::const_iterator limit;
  for (limit = text.end(), cursor = text.begin(); cursor != limit; ++cursor) {
    char character = *cursor;
    if (isalnum(character)) {
      result += character;
    } else {
      switch (character) {
        case '-':
        case '.':
        case '_':
        case '~':
          result += character;
          break;
        default:
          unsigned char byte = static_cast<unsigned char>(character);
          result = result + '%' + kHexDigits[byte / kHexBase] +
              kHexDigits[byte % kHexBase];
      }
    }
  }
  return result;
}

std::string BuildBaseString(const GURL& request_base_url,
                            OAuthRequestSigner::HttpMethod http_method,
                            const std::string base_parameters) {
  return StringPrintf("%s&%s&%s",
                      HttpMethodName(http_method).c_str(),
                      EncodedOAuthParameter(request_base_url.spec()).c_str(),
                      EncodedOAuthParameter(base_parameters).c_str());
}

std::string BuildBaseStringParameters(
    const OAuthRequestSigner::Parameters& parameters) {
  std::string result = "";
  OAuthRequestSigner::Parameters::const_iterator cursor;
  OAuthRequestSigner::Parameters::const_iterator limit;
  bool first = true;
  for (cursor = parameters.begin(), limit = parameters.end();
       cursor != limit;
       ++cursor) {
    if (first) {
      first = false;
    } else {
      result += '&';
    }
    result += EncodedOAuthParameter(cursor->first);
    result += '=';
    result += EncodedOAuthParameter(cursor->second);
  }
  return result;
}

std::string GenerateNonce() {
  char result[kMaxNonceLength + 1];
  int length = base::RandUint64() % (kMaxNonceLength - kMinNonceLength + 1) +
      kMinNonceLength;
  result[length] = '\0';
  for (int index = 0; index < length; ++index)
    result[index] = kOAuthNonceCharacters[
        base::RandUint64() % (sizeof(kOAuthNonceCharacters) - 1)];
  return result;
}

std::string GenerateTimestamp() {
  return base::StringPrintf(
      "%" PRId64,
      (base::Time::NowFromSystemTime() - base::Time::UnixEpoch()).InSeconds());
}

// Creates a string-to-string, keyword-value map from a parameter/query string
// that uses ampersand (&) to seperate paris and equals (=) to seperate
// keyword from value.
bool ParseQuery(const std::string& query,
                OAuthRequestSigner::Parameters* parameters_result) {
  std::string::const_iterator cursor;
  std::string keyword;
  std::string::const_iterator limit;
  OAuthRequestSigner::Parameters parameters;
  ParseQueryState state;
  std::string value;

  state = START_STATE;
  for (cursor = query.begin(), limit = query.end();
       cursor != limit;
       ++cursor) {
    char character = *cursor;
    switch (state) {
      case KEYWORD_STATE:
        switch (character) {
          case '&':
            parameters[keyword] = value;
            keyword = "";
            value = "";
            state = START_STATE;
            break;
          case '=':
            state = VALUE_STATE;
            break;
          default:
            keyword += character;
        }
        break;
      case START_STATE:
        switch (character) {
          case '&':  // Intentionally falling through
          case '=':
            return false;
          default:
            keyword += character;
            state = KEYWORD_STATE;
        }
        break;
      case VALUE_STATE:
        switch (character) {
          case '=':
            return false;
          case '&':
            parameters[keyword] = value;
            keyword = "";
            value = "";
            state = START_STATE;
            break;
          default:
            value += character;
        }
        break;
    }
  }
  switch (state) {
    case START_STATE:
      break;
    case KEYWORD_STATE:  // Intentionally falling through
    case VALUE_STATE:
      parameters[keyword] = value;
      break;
    default:
      NOTREACHED();
  }
  *parameters_result = parameters;
  return true;
}

// Creates the value for the oauth_signature parameter when the
// oauth_signature_method is HMAC-SHA1.
bool SignHmacSha1(const std::string& text,
                  const std::string& key,
                  std::string* signature_return) {
  crypto::HMAC hmac(crypto::HMAC::SHA1);
  DCHECK(hmac.DigestLength() == kHmacDigestLength);
  unsigned char digest[kHmacDigestLength];
  hmac.Init(key);
  bool result = hmac.Sign(text, digest, kHmacDigestLength) &&
      base::Base64Encode(std::string(reinterpret_cast<const char*>(digest),
                                     kHmacDigestLength),
                         signature_return);
  return result;
}

// Creates the value for the oauth_signature parameter when the
// oauth_signature_method is PLAINTEXT.
//
// Not yet implemented, and might never be.
bool SignPlaintext(const std::string& text,
                   const std::string& key,
                   std::string* result) {
  NOTIMPLEMENTED();
  return false;
}

// Creates the value for the oauth_signature parameter when the
// oauth_signature_method is RSA-SHA1.
//
// Not yet implemented, and might never be.
bool SignRsaSha1(const std::string& text,
                 const std::string& key,
                 std::string* result) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace

// static
bool OAuthRequestSigner::ParseAndSign(const GURL& request_url_with_parameters,
                                      SignatureMethod signature_method,
                                      HttpMethod http_method,
                                      const std::string& consumer_key,
                                      const std::string& consumer_secret,
                                      const std::string& token_key,
                                      const std::string& token_secret,
                                      std::string* result) {
  DCHECK(request_url_with_parameters.is_valid());
  Parameters parameters;
  if (request_url_with_parameters.has_query()) {
    const std::string& query = request_url_with_parameters.query();
    if (!query.empty()) {
      if (!ParseQuery(query, &parameters))
        return false;
    }
  }
  std::string spec = request_url_with_parameters.spec();
  std::string url_without_parameters = spec;
  std::string::size_type question = spec.find("?");
  if (question != std::string::npos) {
    url_without_parameters = spec.substr(0,question);
  }
  return Sign (GURL(url_without_parameters), parameters, signature_method,
               http_method, consumer_key, consumer_secret, token_key,
               token_secret, result);
}

// Returns a copy of request_parameters, with parameters that are required by
// OAuth added as needed.
OAuthRequestSigner::Parameters
PrepareParameters(const OAuthRequestSigner::Parameters& request_parameters,
                  OAuthRequestSigner::SignatureMethod signature_method,
                  OAuthRequestSigner::HttpMethod http_method,
                  const std::string& consumer_key,
                  const std::string& token_key) {
  OAuthRequestSigner::Parameters result(request_parameters);

  if (result.find(kOAuthNonceLabel) == result.end())
    result[kOAuthNonceLabel] = GenerateNonce();

  if (result.find(kOAuthTimestampLabel) == result.end())
    result[kOAuthTimestampLabel] = GenerateTimestamp();

  result[kOAuthConsumerKeyLabel] = consumer_key;
  result[kOAuthSignatureMethodLabel] = SignatureMethodName(signature_method);
  result[kOAuthTokenLabel] = token_key;
  result[kOAuthVersionLabel] = kOAuthVersion;

  return result;
}

// static
bool OAuthRequestSigner::Sign(
    const GURL& request_base_url,
    const Parameters& request_parameters,
    SignatureMethod signature_method,
    HttpMethod http_method,
    const std::string& consumer_key,
    const std::string& consumer_secret,
    const std::string& token_key,
    const std::string& token_secret,
    std::string* signed_text_return) {
  DCHECK(request_base_url.is_valid());
  Parameters parameters = PrepareParameters(request_parameters,
                                            signature_method,
                                            http_method,
                                            consumer_key,
                                            token_key);
  std::string base_parameters = BuildBaseStringParameters(parameters);
  std::string base = BuildBaseString(request_base_url,
                                     http_method,
                                     base_parameters);
  std::string key = consumer_secret + '&' + token_secret;
  bool is_signed = false;
  std::string signature;
  switch (signature_method) {
    case HMAC_SHA1_SIGNATURE:
      is_signed =  SignHmacSha1(base, key, &signature);
      break;
    case RSA_SHA1_SIGNATURE:
      is_signed =  SignRsaSha1(base, key, &signature);
      break;
    case PLAINTEXT_SIGNATURE:
      is_signed =  SignPlaintext(base, key, &signature);
      break;
    default:
      NOTREACHED();
  }
  if (is_signed) {
    std::string signed_text;
    switch (http_method) {
      case GET_METHOD:
        signed_text = request_base_url.spec() + '?';
        // Intentionally falling through
      case POST_METHOD:
        signed_text += base_parameters + '&' + kOAuthSignatureLabel + '=' +
            EncodedOAuthParameter(signature);
        break;
      default:
        NOTREACHED();
    }
    *signed_text_return = signed_text;
  }
  return is_signed;
}
