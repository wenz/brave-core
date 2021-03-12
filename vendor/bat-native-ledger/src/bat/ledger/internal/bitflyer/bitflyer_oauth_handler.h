/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_HANDLER_H_

#include "bat/ledger/internal/core/bat_ledger_context.h"

namespace ledger {

struct OAuthParams {};

struct OAuthResult {};

class BitflyerOAuthHandler : public BATLedgerContext::Component {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  explicit BitflyerOAuthHandler(BATLedgerContext* context);

  ~BitflyerOAuthHandler();

  AsyncResult<OAuthResult> HandleOAuthRedirect(
      const mojom::OAuthRedirectParams& params);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_OAUTH_HANDLER_H_
