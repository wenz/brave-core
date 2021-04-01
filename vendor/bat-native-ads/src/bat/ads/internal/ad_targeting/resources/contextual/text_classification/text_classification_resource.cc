/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/resources/contextual/text_classification/text_classification_resource.h"

#include "base/json/json_reader.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace ad_targeting {
namespace resource {

namespace {
const char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}

TextClassification::TextClassification() {
  text_processing_pipeline_.reset(
      ml::pipeline::TextProcessing::CreateInstance());
}

TextClassification::~TextClassification() = default;

bool TextClassification::IsInitialized() const {
  return text_processing_pipeline_ &&
         text_processing_pipeline_->IsInitialized();
}

void TextClassification::Load() {
  AdsClientHelper::Get()->LoadUserModelForId(
      kResourceId, [=](const Result result, const std::string& json) {
    text_processing_pipeline_.reset(
        ml::pipeline::TextProcessing::CreateInstance());

    if (result != SUCCESS) {
      BLOG(1, "Failed to load " << kResourceId
          << " text classification resource");
      return;
    }

    BLOG(1, "Successfully loaded " << kResourceId
        << " text classification resource");

    if (!text_processing_pipeline_->FromJson(json)) {
      BLOG(1, "Failed to initialize " << kResourceId
          << " text classification resource");
      return;
    }

    BLOG(1,
         "Successfully initialized " << kResourceId
            << " text classification resource");
  });
}

ml::pipeline::TextProcessing* TextClassification::get() const {
  return text_processing_pipeline_.get();
}

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads
