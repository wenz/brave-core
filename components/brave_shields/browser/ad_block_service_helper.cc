/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"

#include <algorithm>
#include <utility>

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/sha2.h"

using adblock::FilterList;

namespace brave_shields {

std::vector<FilterList>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterList>& region_lists,
    const std::string& uuid) {
  std::string uuid_uppercase = base::ToUpperASCII(uuid);
  return std::find_if(region_lists.begin(), region_lists.end(),
                      [&uuid_uppercase](const FilterList& filter_list) {
                        return filter_list.uuid == uuid_uppercase;
                      });
}

std::vector<FilterList>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<FilterList>& region_lists,
    const std::string& locale) {
  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }
  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  return std::find_if(
      region_lists.begin(), region_lists.end(),
      [&adjusted_locale](const FilterList& filter_list) {
        return std::find_if(filter_list.langs.begin(), filter_list.langs.end(),
                            [adjusted_locale](const std::string& lang) {
                              return lang == adjusted_locale;
                            }) != filter_list.langs.end();
      });
}

std::vector<FilterList> RegionalCatalogFromJSON(
    const std::string& catalog_json) {
  std::vector<adblock::FilterList> catalog = std::vector<adblock::FilterList>();

  base::Optional<base::Value> regional_lists =
      base::JSONReader::Read(catalog_json);
  if (!regional_lists) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  for (auto i = regional_lists->GetList().begin();
          i < regional_lists->GetList().end(); i++) {
    const auto* uuid = i->FindKey("uuid");
    if (!uuid || !uuid->is_string()) {
      continue;
    }
    const auto* url = i->FindKey("url");
    if (!url || !url->is_string()) {
      continue;
    }
    const auto* title = i->FindKey("title");
    if (!title || !title->is_string()) {
      continue;
    }
    std::vector<std::string> langs = std::vector<std::string>();
    const auto* langs_key = i->FindKey("langs");
    if (!langs_key || !langs_key->is_list()) {
      continue;
    }
    for (auto lang = langs_key->GetList().begin();
            lang < langs_key->GetList().end(); lang++) {
      if (!lang->is_string()) {
        continue;
      }
      langs.push_back(lang->GetString());
    }
    const auto* support_url = i->FindKey("support_url");
    if (!support_url || !support_url->is_string()) {
      continue;
    }
    const auto* component_id = i->FindKey("component_id");
    if (!component_id || !component_id->is_string()) {
      continue;
    }
    const auto* base64_public_key = i->FindKey("base64_public_key");
    if (!base64_public_key || !base64_public_key->is_string()) {
      continue;
    }
    const auto* desc = i->FindKey("desc");
    if (!desc || !desc->is_string()) {
      continue;
    }

    catalog.push_back(adblock::FilterList(uuid->GetString(),
                                          url->GetString(),
                                          title->GetString(),
                                          langs,
                                          support_url->GetString(),
                                          component_id->GetString(),
                                          base64_public_key->GetString(),
                                          desc->GetString()));
  }

  return catalog;
}

// Merges the contents of the second UrlCosmeticResources Value into the first
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void MergeResourcesInto(base::Value from, base::Value* into, bool force_hide) {
  base::Value* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into->FindKey("force_hide_selectors");
    if (!resources_hide_selectors || !resources_hide_selectors->is_list()) {
        into->SetKey("force_hide_selectors", base::ListValue());
        resources_hide_selectors = into->FindKey("force_hide_selectors");
    }
  } else {
    resources_hide_selectors = into->FindKey("hide_selectors");
  }
  base::Value* from_resources_hide_selectors =
      from.FindKey("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto i = from_resources_hide_selectors->GetList().begin();
            i < from_resources_hide_selectors->GetList().end();
            i++) {
      resources_hide_selectors->Append(std::move(*i));
    }
  }

  base::Value* resources_style_selectors = into->FindKey("style_selectors");
  base::Value* from_resources_style_selectors =
      from.FindKey("style_selectors");
  if (resources_style_selectors && from_resources_style_selectors) {
    for (auto i : from_resources_style_selectors->DictItems()) {
      base::Value* resources_entry =
          resources_style_selectors->FindKey(i.first);
      if (resources_entry) {
        for (auto j = i.second.GetList().begin();
                j < i.second.GetList().end();
                j++) {
          resources_entry->Append(std::move(*j));
        }
      } else {
        resources_style_selectors->SetKey(i.first, std::move(i.second));
      }
    }
  }

  base::Value* resources_exceptions = into->FindKey("exceptions");
  base::Value* from_resources_exceptions = from.FindKey("exceptions");
  if (resources_exceptions && from_resources_exceptions) {
    for (auto i = from_resources_exceptions->GetList().begin();
            i < from_resources_exceptions->GetList().end();
            i++) {
      resources_exceptions->Append(std::move(*i));
    }
  }

  base::Value* resources_injected_script = into->FindKey("injected_script");
  base::Value* from_resources_injected_script =
      from.FindKey("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::Value(
            resources_injected_script->GetString()
            + '\n'
            + from_resources_injected_script->GetString());
  }

  base::Value* resources_generichide = into->FindKey("generichide");
  base::Value* from_resources_generichide =
      from.FindKey("generichide");
  if (from_resources_generichide) {
    if (from_resources_generichide->GetBool()) {
      *resources_generichide = base::Value(true);
    }
  }
}

const base::FilePath::CharType kSubscriptionsDir[] = FILE_PATH_LITERAL("FilterListSubscriptionCache");

// Subdirectories are generated by taking the SHA256 hash of the list URL spec,
// then base64 encoding that hash. This generates paths that are:
//     - deterministic
//     - unique
//     - constant length
//     - path-safe
//     - not too long (exactly 45 characters)
base::FilePath DirForCustomSubscription(const SubscriptionIdentifier id) {
  const std::string hash = crypto::SHA256HashString(id.spec());

  std::string pathsafe_id;
  base::Base64UrlEncode(hash, base::Base64UrlEncodePolicy::INCLUDE_PADDING, &pathsafe_id);

  base::FilePath user_data_dir;
  DCHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
  DCHECK(!user_data_dir.empty());
  return user_data_dir.AppendASCII(kSubscriptionsDir).AppendASCII(pathsafe_id);
}

}  // namespace brave_shields
