/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_oauth_handler.h"

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

namespace ledger {

namespace {

class LoginTask : public BATLedgerTask<OAuthResponse> {
 public:
  explicit LoginTask(BATLedgerContext* context) : BATLedgerTask(context) {}

  void Start(const base::flat_map<std::string, std::string>& params) {}
};

}  // namespace

BitflyerOAuthHandler::BitflyerOAuthHandler(BATLedgerContext* context)
    : Component(context) {}

BitflyerOAuthHandler::~BitflyerOAuthHandler() = default;

void BitflyerOAuthHandler::HandleOAuthRedirect(
    const base::flat_map<std::string, std::string>& params) {
  auto wallet = context()->GetLedgerImpl()->wallet()->GetWallet();
  if (!wallet) {
    context BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto bitflyer_wallet = GetWallet(ledger_);
  if (!bitflyer_wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = bitflyer_wallet->one_time_string;

  // we need to generate new string as soon as authorization is triggered
  bitflyer_wallet->one_time_string = GenerateRandomString(ledger::is_testing);
  const bool success = ledger_->bitflyer()->SetWallet(bitflyer_wallet->Clone());

  if (!success) {
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto it = args.find("error_description");
  if (it != args.end()) {
    const std::string message = args.at("error_description");
    BLOG(1, message);
    if (message == "User does not meet minimum requirements") {
      callback(type::Result::NOT_FOUND, {});
      return;
    }

    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    BLOG(0, "Code is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty()) {
    BLOG(0, "One time string is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string mismatch");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  ledger_->context()
      ->Get<BitflyerOAuthEndpoint>()
      ->RequestAuthorization(wallet->payment_id, code)
      .Then(base::BindOnce(&BitflyerAuthorization::OnAuthorize,
                           base::Unretained(this), callback));
}

void BitflyerAuthorization::OnAuthorize(
    ExternalWalletAuthorizationCallback callback,
    const BitflyerOAuthResponse& response) {
  if (response.error == BitflyerOAuthError::kExpiredToken) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN, {});
    ledger_->bitflyer()->DisconnectWallet();
    return;
  }

  if (response.error != BitflyerOAuthError::kSuccess) {
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  DCHECK(!response.access_token.empty());
  DCHECK(!response.address.empty());
  DCHECK(!response.linking_info.empty());

  auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  ledger_->context()
      ->Get<BitflyerLinkingEndpoint>()
      ->RequestLink(wallet->payment_id, wallet->recovery_seed,
                    response.linking_info)
      .Then(base::BindOnce(&BitflyerAuthorization::OnClaimWallet,
                           base::Unretained(this), response.access_token,
                           response.address, callback));
}

void BitflyerAuthorization::OnClaimWallet(
    const std::string& token,
    const std::string& address,
    ExternalWalletAuthorizationCallback callback,
    const BitflyerLinkingResponse& response) {
  switch (response.error) {
    case BitflyerLinkingError::kSuccess:
      break;
    case BitflyerLinkingError::kLimitReached:
      callback(type::Result::ALREADY_EXISTS, {});
      return;
    default:
      callback(type::Result::LEDGER_ERROR, {});
      return;
  }

  auto wallet_ptr = GetWallet(ledger_);

  wallet_ptr->token = token;
  wallet_ptr->address = address;

  switch (wallet_ptr->status) {
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED:
    case type::WalletStatus::DISCONNECTED_VERIFIED:
      wallet_ptr->status = type::WalletStatus::VERIFIED;
      break;
    default:
      break;
  }

  ledger_->bitflyer()->SetWallet(wallet_ptr->Clone());

  callback(type::Result::LEDGER_OK, {});
}

}  // namespace ledger
