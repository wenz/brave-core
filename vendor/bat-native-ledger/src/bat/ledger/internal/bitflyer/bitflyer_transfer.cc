/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bitflyer/bitflyer_transfer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace bitflyer {

BitflyerTransfer::BitflyerTransfer(LedgerImpl* ledger) : ledger_(ledger) {}

BitflyerTransfer::~BitflyerTransfer() = default;

void BitflyerTransfer::Start(const Transaction& transaction,
                             client::TransactionCallback callback) {
  auto wallet = GetWallet(ledger_);
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  ledger_->context()
      ->Get<BitflyerTransferEndpoint>()
      ->RequestTransfer(wallet->token, transaction.address, transaction.amount)
      .Then(base::BindOnce(&BitflyerTransfer::OnCreateTransaction,
                           base::Unretained(this), callback));
}

void BitflyerTransfer::OnCreateTransaction(
    client::TransactionCallback callback,
    const BitflyerTransferResponse& response) {
  if (response.error == BitflyerTransferError::kExpiredToken) {
    callback(type::Result::EXPIRED_TOKEN, "");
    ledger_->bitflyer()->DisconnectWallet();
    return;
  }

  if (response.error != BitflyerTransferError::kSuccess) {
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(type::Result::LEDGER_OK, response.transfer_id);
}

}  // namespace bitflyer
}  // namespace ledger
