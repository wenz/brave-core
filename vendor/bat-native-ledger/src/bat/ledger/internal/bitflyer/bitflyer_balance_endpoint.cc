/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_balance_endpoint.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/internal/bitflyer/bitflyer_settings.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "net/http/http_status_code.h"

namespace ledger {

namespace {

const char kEndpointPath[] = "/api/link/v1/account/inventory";

class GetBalanceJob : public BATLedgerJob<BitflyerBalanceResponse> {
 public:
  void Start(const std::string& access_token) {
    auto& settings = context()->GetSettings<BitflyerSettings>();

    auto request = mojom::UrlRequest::New();
    request->url = std::string(settings.base_url) + kEndpointPath;
    request->headers = {"Authorization: Bearer " + access_token};

    context()
        ->Get<URLFetcher>()
        ->Fetch(std::move(request))
        .Then(
            base::BindOnce(&GetBalanceJob::OnResponse, base::AsWeakPtr(this)));
  }

 private:
  using Response = BitflyerBalanceResponse;

  Response ParseResponse(const mojom::UrlResponsePtr& response) {
    switch (response->status_code) {
      case net::HTTP_OK:
        break;
      case net::HTTP_UNAUTHORIZED:
      case net::HTTP_NOT_FOUND:
      case net::HTTP_FORBIDDEN:
        return Response(BitflyerBalanceError::kExpiredToken);
      default:
        return Response(BitflyerBalanceError::kUnexpectedError);
    }

    auto root = base::JSONReader::Read(response->body);
    if (!root || !root->is_dict())
      return Response(BitflyerBalanceError::kInvalidResponse);

    auto* inventory = root->FindListKey("inventory");
    if (!inventory) {
      context()->LogError(FROM_HERE) << "Missing inventory";
      return Response(BitflyerBalanceError::kInvalidResponse);
    }

    for (auto& item : inventory->GetList()) {
      auto* currency_code = item.FindStringKey("currency_code");
      if (currency_code && *currency_code == "BAT") {
        if (auto available = item.FindDoubleKey("available"))
          return Response(*available);

        context()->LogError(FROM_HERE) << "Missing available";
        return Response(BitflyerBalanceError::kInvalidResponse);
      }
    }

    context()->LogError(FROM_HERE) << "Missing BAT in inventory";
    return Response(BitflyerBalanceError::kInvalidResponse);
  }

  void OnResponse(const mojom::UrlResponsePtr& response) {
    resolver().Complete(ParseResponse(response));
  }
};

}  // namespace

BitflyerBalanceResponse::BitflyerBalanceResponse(double balance)
    : balance(balance) {}

BitflyerBalanceResponse::BitflyerBalanceResponse(BitflyerBalanceError error)
    : error(error) {}

BitflyerBalanceResponse::~BitflyerBalanceResponse() = default;

const BATLedgerContext::ComponentKey BitflyerBalanceEndpoint::kComponentKey;

AsyncResult<BitflyerBalanceResponse> BitflyerBalanceEndpoint::RequestBalance(
    const std::string& access_token) {
  return RequestBalanceImpl(access_token);
}

AsyncResult<BitflyerBalanceResponse>
BitflyerBalanceEndpoint::RequestBalanceImpl(const std::string& access_token) {
  return context()->StartJob<GetBalanceJob>(access_token);
}

}  // namespace ledger
