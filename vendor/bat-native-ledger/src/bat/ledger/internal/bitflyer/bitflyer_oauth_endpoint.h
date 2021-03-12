/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_ENDPOINT_H_

#include <string>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

// POST https://bitflyer.jp/api/link/v1/token
//
// Request body:
// {
//   "client_id": "abcdedg",
//   "client_secret": "xxxxxxxxxxxxxxxxxx",
//   "code": "xxxxxxxxxxxxxxxxxxxxxxxxxx",
//   "grant_type": "code",
//   "code_verifier": "xxxxxxx",
//   "expires_in": 3600,
//   "external_account_id": "xxxxxxxxxx",
//   "request_id": "xxxxxxxx",
//   "request_deposit_id": true
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "access_token": "xxxxbbbbccccddddeeeeqqqq",
//   "refresh_token": "yyyyyyyyyyyyyyyyyyyyyyyyyy",
//   "expires_in": 302010,
//   "account_hash": "xxxxxxxxxxxxxxxxxx",
//   "token_type": "Bearer",
//   "deposit_id": "xxxxxxxxx",
//   "linking_info": "xxxxx"
// }

namespace ledger {

enum class BitflyerOAuthError {
  kSuccess,
  kExpiredToken,
  kInvalidResponse,
  kUnexpectedError
};

struct BitflyerOAuthResponse {
  explicit BitflyerOAuthResponse(BitflyerOAuthError error);
  explicit BitflyerOAuthResponse(std::string access_token,
                                 std::string address,
                                 std::string linking_info);
  ~BitflyerOAuthResponse();

  BitflyerOAuthResponse(const BitflyerOAuthResponse&);
  BitflyerOAuthResponse& operator=(const BitflyerOAuthResponse&);

  BitflyerOAuthError error = BitflyerOAuthError::kSuccess;
  std::string access_token;
  std::string address;
  std::string linking_info;
};

class BitflyerOAuthEndpoint : public BATLedgerContext::Object {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  AsyncResult<BitflyerOAuthResponse> RequestAccessToken(
      const std::string& payment_id,
      const std::string& oauth_code);

 protected:
  virtual AsyncResult<BitflyerOAuthResponse> RequestAccessTokenImpl(
      const std::string& payment_id,
      const std::string& oauth_code);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_ENDPOINT_H_
