/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_ENDPOINT_H_

#include <string>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

// POST https://bitflyer.jp/api/link/v1/coin/withdraw-to-deposit-id/request
//
// Request body:
// {
//   "currency_code": "BAT",
//   "amount": "1.00",
//   "dry_run": true,
//   "deposit_id": "xxxxxxxxx",
//   "transfer_id": "base58idgoeshere",
//   "dry_run_option": {
//     "request_api_transfer_status": "INVALID_AMOUNT",
//     "process_time_sec": 5,
//     "status_api_transfer_status": "INVALID_AMOUNT"
//   }
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
// HTTP_NOT_FOUND (404)
//
// Response body:
// {
//   "currency_code": "BAT",
//   "amount": "1.00",
//   "dry_run": true,
//   "message": null,
//   "transfer_id": "base58idgoeshere",
//   "transfer_status": "SUCCESS"
// }

namespace ledger {

enum class BitflyerTransferError {
  kSuccess,
  kExpiredToken,
  kAccountNotFound,
  kInvalidResponse,
  kTransferFailed,
  kUnexpectedError
};

struct BitflyerTransferResponse {
  explicit BitflyerTransferResponse(std::string transfer_id);
  explicit BitflyerTransferResponse(BitflyerTransferError error);
  ~BitflyerTransferResponse();

  BitflyerTransferError error = BitflyerTransferError::kSuccess;
  std::string transfer_id;
};

class BitflyerTransferEndpoint : public BATLedgerContext::Object {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  AsyncResult<BitflyerTransferResponse> RequestTransfer(
      const std::string& access_token,
      const std::string& destination_address,
      double amount);

 protected:
  virtual AsyncResult<BitflyerTransferResponse> RequestTransferImpl(
      const std::string& access_token,
      const std::string& destination_address,
      double amount);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_ENDPOINT_H_
