// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_FAKE_OAUTH2_TOKEN_SERVICE_H_
#define GOOGLE_APIS_GAIA_FAKE_OAUTH2_TOKEN_SERVICE_H_

#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace net {
class URLRequestContextGetter;
}

// Do-nothing implementation of OAuth2TokenService.
class FakeOAuth2TokenService : public OAuth2TokenService {
 public:
  FakeOAuth2TokenService();
  virtual ~FakeOAuth2TokenService();

  void AddAccount(const std::string& account_id);

  void set_request_context(net::URLRequestContextGetter* request_context) {
    request_context_ = request_context;
  }

 protected:
  // OAuth2TokenService overrides.
  virtual void FetchOAuth2Token(RequestImpl* request,
                                const std::string& account_id,
                                net::URLRequestContextGetter* getter,
                                const std::string& client_id,
                                const std::string& client_secret,
                                const ScopeSet& scopes) OVERRIDE;

  virtual void InvalidateOAuth2Token(const std::string& account_id,
                                     const std::string& client_id,
                                     const ScopeSet& scopes,
                                     const std::string& access_token) OVERRIDE;

  virtual bool RefreshTokenIsAvailable(const std::string& account_id) const
      OVERRIDE;

 private:
  // OAuth2TokenService overrides.
  virtual net::URLRequestContextGetter* GetRequestContext() OVERRIDE;

  virtual OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) OVERRIDE;

  std::set<std::string> account_ids_;
  net::URLRequestContextGetter* request_context_;  // weak

  DISALLOW_COPY_AND_ASSIGN(FakeOAuth2TokenService);
};

#endif  // GOOGLE_APIS_GAIA_FAKE_OAUTH2_TOKEN_SERVICE_H_
