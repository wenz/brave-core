/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_settings.h"

namespace ledger {

const BitflyerSettings BitflyerSettings::kProduction = {
    .base_url = "https://bitflyer.jp",
    .client_id = "078bf3871f3741432e813376d996a6a0",
    /* TODO */ .client_secret = "",
    /* TODO */ .transfer_fee_address = ""};

const BitflyerSettings BitflyerSettings::kStaging = {
    .base_url = BITFLYER_STAGING_URL,
    .client_id = "078bf3871f3741432e813376d996a6a0",
    .client_secret = BITFLYER_CLIENT_SECRET,
    .transfer_fee_address = "068e675b-f137-48ed-8068-4ad34ca4f30f"};

const BitflyerSettings BitflyerSettings::kDevelopment =
    BitflyerSettings::kStaging;

}  // namespace ledger
