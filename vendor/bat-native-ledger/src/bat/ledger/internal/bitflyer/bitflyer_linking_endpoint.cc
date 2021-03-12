/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_linking_endpoint.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "net/http/http_status_code.h"

namespace ledger {

namespace {

std::string GetEndpointPath(const std::string& payment_id) {
  return "/v3/wallet/bitflyer/" + payment_id + "/claim";
}

class PostLinkJob : public BATLedgerJob<BitflyerLinkingResponse> {
 public:
  void Start(const std::string& payment_id,
             const std::vector<uint8_t>& recovery_seed,
             const std::string& linking_info) {
    std::string payload = GeneratePayload(linking_info);

    auto request = mojom::UrlRequest::New();
    // TODO(zenparsing): Use settings?
    request->url =
        endpoint::promotion::GetServerUrl(GetEndpointPath(payment_id));
    request->headers = GenerateHeaders(payment_id, recovery_seed, payload);
    request->content_type = "application/json; charset=utf-8";
    request->content = payload;
    request->method = mojom::UrlMethod::POST;

    context()
        ->Get<URLFetcher>()
        ->Fetch(std::move(request))
        .Then(base::BindOnce(&PostLinkJob::OnResponse, base::AsWeakPtr(this)));
  }

 private:
  std::vector<std::string> GenerateHeaders(
      const std::string& payment_id,
      const std::vector<uint8_t>& recovery_seed,
      const std::string& payload) {
    std::string sign_url = "post " + GetEndpointPath(payment_id);
    // TODO(zenparsing): Move this to a better place
    return util::BuildSignHeaders(sign_url, payload, payment_id, recovery_seed);
  }

  std::string GeneratePayload(const std::string& linking_info) {
    base::Value payload(base::Value::Type::DICTIONARY);
    payload.SetStringKey("linkingInfo", linking_info);
    std::string json;
    base::JSONWriter::Write(payload, &json);
    return json;
  }

  using Response = BitflyerLinkingResponse;

  Response ParseResponse(const mojom::UrlResponsePtr& response) {
    switch (response->status_code) {
      case net::HTTP_OK:
        return Response(BitflyerLinkingError::kSuccess);
      case net::HTTP_NOT_FOUND:
        return Response(BitflyerLinkingError::kNotFound);
      case net::HTTP_CONFLICT:
        return Response(BitflyerLinkingError::kLimitReached);
      default:
        return Response(BitflyerLinkingError::kUnexpectedError);
    }
  }

  void OnResponse(const mojom::UrlResponsePtr& response) {
    resolver().Complete(ParseResponse(response));
  }
};

}  // namespace

BitflyerLinkingResponse::BitflyerLinkingResponse(BitflyerLinkingError error)
    : error(error) {}

BitflyerLinkingResponse::~BitflyerLinkingResponse() = default;

const BATLedgerContext::ComponentKey BitflyerLinkingEndpoint::kComponentKey;

AsyncResult<BitflyerLinkingResponse> BitflyerLinkingEndpoint::RequestLink(
    const std::string& payment_id,
    const std::vector<uint8_t>& recovery_seed,
    const std::string& linking_info) {
  return RequestLinkImpl(payment_id, recovery_seed, linking_info);
}

AsyncResult<BitflyerLinkingResponse> BitflyerLinkingEndpoint::RequestLinkImpl(
    const std::string& payment_id,
    const std::vector<uint8_t>& recovery_seed,
    const std::string& linking_info) {
  return context()->StartJob<PostLinkJob>(payment_id, recovery_seed,
                                          linking_info);
}

}  // namespace ledger
