/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_LINKING_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_LINKING_ENDPOINT_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"

// POST /v3/wallet/bitflyer/{payment_id}/claim
//
// Request body:
// {
//   "linkingInfo": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {

enum class BitflyerLinkingError {
  kSuccess,
  kNotFound,
  kLimitReached,
  kUnexpectedError
};

struct BitflyerLinkingResponse {
  explicit BitflyerLinkingResponse(BitflyerLinkingError error);
  ~BitflyerLinkingResponse();

  BitflyerLinkingError error = BitflyerLinkingError::kSuccess;
};

class BitflyerLinkingEndpoint : public BATLedgerContext::Object {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  AsyncResult<BitflyerLinkingResponse> RequestLink(
      const std::string& payment_id,
      const std::vector<uint8_t>& recovery_seed,
      const std::string& linking_info);

 protected:
  virtual AsyncResult<BitflyerLinkingResponse> RequestLinkImpl(
      const std::string& payment_id,
      const std::vector<uint8_t>& recovery_seed,
      const std::string& linking_info);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_LINKING_ENDPOINT_H_
