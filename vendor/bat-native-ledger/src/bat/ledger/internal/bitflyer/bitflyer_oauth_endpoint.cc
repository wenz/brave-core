/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_oauth_endpoint.h"

#include <utility>

#include "base/base64.h"
#include "base/base64url.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/bitflyer/bitflyer_settings.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "crypto/random.h"
#include "crypto/sha2.h"
#include "net/http/http_status_code.h"

namespace ledger {

namespace {

const char kEndpointPath[] = "/api/link/v1/token";
const char kRedirectURL[] = "rewards://bitflyer/authorization";
const int kTokenExpiresInSeconds = 259002;

class PostOAuthJob : public BATLedgerJob<BitflyerOAuthResponse> {
 public:
  void Start(const std::string& payment_id, const std::string& oauth_code) {
    auto& settings = context()->GetSettings<BitflyerSettings>();

    auto request = mojom::UrlRequest::New();
    request->url = std::string(settings.base_url) + kEndpointPath;
    request->content = GeneratePayload(payment_id, oauth_code);
    request->headers = {"Authorization: Basic " + GetAuthCredentials()};
    request->content_type = "application/json";
    request->method = mojom::UrlMethod::POST;

    context()
        ->Get<URLFetcher>()
        ->Fetch(std::move(request))
        .Then(base::BindOnce(&PostOAuthJob::OnResponse, base::AsWeakPtr(this)));
  }

 private:
  std::string GetAuthCredentials() {
    auto& settings = context()->GetSettings<BitflyerSettings>();
    std::string unencoded =
        std::string(settings.client_id) + ":" + settings.client_secret;
    std::string credentials;
    base::Base64Encode(unencoded, &credentials);
    return credentials;
  }

  // TODO(zenparsing): Move this to a randomizer component or similar
  std::string GenerateRandomString() {
    if (ledger::is_testing)
      return "123456789";

    uint8_t bytes[32];
    crypto::RandBytes(bytes, sizeof(bytes));
    return base::HexEncode(bytes, sizeof(bytes));
  }

  std::string GenerateAccountId(const std::string& payment_id) {
    std::string hash = crypto::SHA256HashString(payment_id);
    return base::ToLowerASCII(base::HexEncode(hash.data(), hash.size()));
  }

  std::string GeneratePayload(const std::string& payment_id,
                              const std::string& oauth_code) {
    auto& settings = context()->GetSettings<BitflyerSettings>();

    base::DictionaryValue dict;
    dict.SetStringKey("grant_type", "code");
    dict.SetStringKey("code", oauth_code);
    dict.SetStringKey("client_id", settings.client_id);
    dict.SetStringKey("client_secret", settings.client_secret);
    dict.SetIntKey("expires_in", kTokenExpiresInSeconds);
    dict.SetStringKey("external_account_id", GenerateAccountId(payment_id));
    dict.SetStringKey("request_id", base::GenerateGUID());
    dict.SetStringKey("redirect_uri", kRedirectURL);
    dict.SetBoolKey("request_deposit_id", true);

    // PKCE
    std::string code_verifier = GenerateRandomString();
    std::string code_challenge;
    base::Base64UrlEncode(crypto::SHA256HashString(code_verifier),
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &code_challenge);

    dict.SetStringKey("code_verifier", code_verifier);
    dict.SetStringKey("code_challenge_method", "S256");
    dict.SetStringKey("code_challenge", code_challenge);

    std::string payload;
    base::JSONWriter::Write(dict, &payload);
    return payload;
  }

  using Response = BitflyerOAuthResponse;

  Response ParseResponse(const mojom::UrlResponsePtr& response) {
    switch (response->status_code) {
      case net::HTTP_OK:
        break;
      case net::HTTP_UNAUTHORIZED:
        return Response(BitflyerOAuthError::kExpiredToken);
      default:
        return Response(BitflyerOAuthError::kUnexpectedError);
    }

    auto root = base::JSONReader::Read(response->body);
    if (!root || !root->is_dict())
      return Response(BitflyerOAuthError::kInvalidResponse);

    auto* access_token = root->FindStringKey("access_token");
    if (!access_token || access_token->empty()) {
      context()->LogError(FROM_HERE) << "Missing access_token";
      return Response(BitflyerOAuthError::kInvalidResponse);
    }

    auto* deposit_id = root->FindStringKey("deposit_id");
    if (!deposit_id || deposit_id->empty()) {
      context()->LogError(FROM_HERE) << "Missing deposit_id";
      return Response(BitflyerOAuthError::kInvalidResponse);
    }

    auto* linking_info = root->FindStringKey("linking_info");
    if (!linking_info || linking_info->empty()) {
      context()->LogError(FROM_HERE) << "Missing linking_info";
      return Response(BitflyerOAuthError::kInvalidResponse);
    }

    return Response(std::move(*access_token), std::move(*deposit_id),
                    std::move(*linking_info));
  }

  void OnResponse(const mojom::UrlResponsePtr& response) {
    resolver().Complete(ParseResponse(response));
  }
};

}  // namespace

BitflyerOAuthResponse::BitflyerOAuthResponse(std::string access_token,
                                             std::string address,
                                             std::string linking_info)
    : access_token(std::move(access_token)),
      address(std::move(address)),
      linking_info(std::move(linking_info)) {}

BitflyerOAuthResponse::BitflyerOAuthResponse(BitflyerOAuthError error)
    : error(error) {}

BitflyerOAuthResponse::~BitflyerOAuthResponse() = default;

BitflyerOAuthResponse::BitflyerOAuthResponse(const BitflyerOAuthResponse&) =
    default;

BitflyerOAuthResponse& BitflyerOAuthResponse::operator=(
    const BitflyerOAuthResponse&) = default;

const BATLedgerContext::ComponentKey BitflyerOAuthEndpoint::kComponentKey;

AsyncResult<BitflyerOAuthResponse> BitflyerOAuthEndpoint::RequestAccessToken(
    const std::string& payment_id,
    const std::string& oauth_code) {
  return RequestAccessTokenImpl(payment_id, oauth_code);
}

AsyncResult<BitflyerOAuthResponse>
BitflyerOAuthEndpoint::RequestAccessTokenImpl(const std::string& payment_id,
                                              const std::string& oauth_code) {
  return context()->StartJob<PostOAuthJob>(payment_id, oauth_code);
}

}  // namespace ledger
