/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_transfer_endpoint.h"

#include <utility>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/bitflyer/bitflyer_settings.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "net/http/http_status_code.h"

namespace ledger {

namespace {

const char kEndpointPath[] = "/api/link/v1/coin/withdraw-to-deposit-id/request";

class PostTransferJob : public BATLedgerJob<BitflyerTransferResponse> {
 public:
  void Start(const std::string& access_token,
             const std::string& destination_address,
             double amount) {
    auto& settings = context()->GetSettings<BitflyerSettings>();

    auto request = mojom::UrlRequest::New();
    request->url = std::string(settings.base_url) + kEndpointPath;
    request->headers = {"Authorization: Bearer " + access_token};
    request->content_type = "application/json; charset=utf-8";
    request->content = GeneratePayload(destination_address, amount);
    request->method = mojom::UrlMethod::POST;

    context()
        ->Get<URLFetcher>()
        ->Fetch(std::move(request))
        .Then(base::BindOnce(&PostTransferJob::OnResponse,
                             base::AsWeakPtr(this)));
  }

 private:
  std::string GeneratePayload(const std::string& destination_address,
                              double amount) {
    base::Value payload(base::Value::Type::DICTIONARY);
    payload.SetStringKey("currency_code", "BAT");
    payload.SetStringKey("amount", base::NumberToString(amount));
    payload.SetBoolKey("dry_run", false);
    payload.SetStringKey("deposit_id", destination_address);
    payload.SetStringKey("transfer_id", base::GenerateGUID());

    std::string json;
    base::JSONWriter::Write(payload, &json);
    return json;
  }

  using Response = BitflyerTransferResponse;

  Response ParseResponse(const mojom::UrlResponsePtr& response) {
    switch (response->status_code) {
      case net::HTTP_OK:
        break;
      case net::HTTP_UNAUTHORIZED:
        return Response(BitflyerTransferError::kExpiredToken);
      case net::HTTP_NOT_FOUND:
        return Response(BitflyerTransferError::kAccountNotFound);
      default:
        return Response(BitflyerTransferError::kUnexpectedError);
    }

    auto root = base::JSONReader::Read(response->body);
    if (!root || !root->is_dict())
      return Response(BitflyerTransferError::kInvalidResponse);

    auto* transfer_id = root->FindStringKey("transfer_id");
    if (!transfer_id) {
      context()->LogError(FROM_HERE) << "Missing transfer id";
      return Response(BitflyerTransferError::kInvalidResponse);
    }

    auto* transfer_status = root->FindStringKey("transfer_status");
    if (!transfer_status) {
      context()->LogError(FROM_HERE) << "Missing transfer status";
      return Response(BitflyerTransferError::kInvalidResponse);
    }

    if (*transfer_status != "SUCCESS") {
      context()->LogError(FROM_HERE)
          << "Transfer failed (status: " << *transfer_status << ")";

      if (auto* message = root->FindStringKey("message"))
        context()->LogError(FROM_HERE) << *message;

      return Response(BitflyerTransferError::kTransferFailed);
    }

    return Response(std::move(*transfer_id));
  }

  void OnResponse(const mojom::UrlResponsePtr& response) {
    resolver().Complete(ParseResponse(response));
  }
};

}  // namespace

BitflyerTransferResponse::BitflyerTransferResponse(std::string transfer_id)
    : transfer_id(std::move(transfer_id)) {}

BitflyerTransferResponse::BitflyerTransferResponse(BitflyerTransferError error)
    : error(error) {}

BitflyerTransferResponse::~BitflyerTransferResponse() = default;

const BATLedgerContext::ComponentKey BitflyerTransferEndpoint::kComponentKey;

AsyncResult<BitflyerTransferResponse> BitflyerTransferEndpoint::RequestTransfer(
    const std::string& access_token,
    const std::string& destination_address,
    double amount) {
  return RequestTransferImpl(access_token, destination_address, amount);
}

AsyncResult<BitflyerTransferResponse>
BitflyerTransferEndpoint::RequestTransferImpl(
    const std::string& access_token,
    const std::string& destination_address,
    double amount) {
  return context()->StartJob<PostTransferJob>(access_token, destination_address,
                                              amount);
}

}  // namespace ledger
