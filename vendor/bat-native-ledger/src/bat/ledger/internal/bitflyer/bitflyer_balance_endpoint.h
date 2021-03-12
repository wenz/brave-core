/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_BALANCE_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_BALANCE_ENDPOINT_H_

#include <string>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

// GET https://bitflyer.jp/api/link/v1/account/inventory
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "account_hash": "0123456789",
//   "inventory": [
//     {
//       "currency_code": "JPY",
//       "amount": 1024078,
//       "available": 508000
//     },
//     {
//       "currency_code": "BTC",
//       "amount": 10.24,
//       "available": 4.12
//     },
//     {
//       "currency_code": "ETH",
//       "amount": 10.24,
//       "available": 4.12
//     }
//   ]
// }

namespace ledger {

enum class BitflyerBalanceError {
  kSuccess,
  kExpiredToken,
  kInvalidResponse,
  kUnexpectedError
};

struct BitflyerBalanceResponse {
  explicit BitflyerBalanceResponse(BitflyerBalanceError error);
  explicit BitflyerBalanceResponse(double balance);
  ~BitflyerBalanceResponse();

  BitflyerBalanceError error = BitflyerBalanceError::kSuccess;
  double balance = 0;
};

class BitflyerBalanceEndpoint : public BATLedgerContext::Object {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  AsyncResult<BitflyerBalanceResponse> RequestBalance(
      const std::string& access_token);

 protected:
  virtual AsyncResult<BitflyerBalanceResponse> RequestBalanceImpl(
      const std::string& access_token);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_BALANCE_ENDPOINT_H_
