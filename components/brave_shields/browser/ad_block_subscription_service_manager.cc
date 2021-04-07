/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_service_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/download/brave_download_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_subscription_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_shields {

void AdBlockSubscriptionServiceManager::OnSystemProfileCreated(Profile* profile, Profile::CreateStatus status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(profile->IsSystemProfile());
  DCHECK_NE(status, Profile::CREATE_STATUS_LOCAL_FAIL);
  if (status != Profile::CREATE_STATUS_INITIALIZED) {
    return;
  }

  InitializeDownloadManager(profile);
}

AdBlockSubscriptionServiceManager::AdBlockSubscriptionServiceManager(
    brave_component_updater::BraveComponent::Delegate* delegate)
    : delegate_(delegate),
      initialized_(false) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  DCHECK(profile_manager);

  auto system_profile_path = ProfileManager::GetSystemProfilePath();
  auto* profile = profile_manager->GetProfileByPath(system_profile_path);

  PrefService* local_state = g_browser_process->local_state();
  if (!local_state) {
    return;
  }
  // TODO - only start download manager if at least one list is enabled

  if (profile) {
    InitializeDownloadManager(profile);
  } else {
    DCHECK(!profile_manager->GetLoadedProfiles().empty());

    // Force the system profile to be created. Without this call, it is
    // eventually lazy-loaded by other services by the same mechanism.
    g_browser_process->profile_manager()->CreateProfileAsync(
          ProfileManager::GetSystemProfilePath(),
          base::BindRepeating(&AdBlockSubscriptionServiceManager::OnSystemProfileCreated,
                              weak_ptr_factory_.GetWeakPtr()),
          /*name=*/base::string16(), /*icon_url=*/std::string());
  }
}

AdBlockSubscriptionServiceManager::~AdBlockSubscriptionServiceManager() {
}

void AdBlockSubscriptionServiceManager::CreateSubscription(const GURL list_url) {
  auto subscription_service = AdBlockSubscriptionServiceFactory(list_url, delegate_);
  subscription_service->Start();

  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockSubscriptionServiceManager::UpdateFilterListPrefs,
                     base::Unretained(this), list_url, subscription_service->GetInfo()));

  subscription_services_.insert(std::make_pair(list_url, std::move(subscription_service)));

  // TODO - start download manager if it has not yet been started (first list created)

  // TODO start download
}

std::vector<FilterListSubscriptionInfo> AdBlockSubscriptionServiceManager::GetSubscriptions() const {
  auto infos = std::vector<FilterListSubscriptionInfo>();

  for (const auto& subscription_service : subscription_services_) {
    infos.push_back(subscription_service.second->GetInfo());
  }

  return infos;
}

void AdBlockSubscriptionServiceManager::EnableSubscription(const SubscriptionIdentifier& id, bool enabled) {
  auto it = subscription_services_.find(id);
  DCHECK(it != subscription_services_.end());
  it->second->SetEnabled(enabled);

  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockSubscriptionServiceManager::UpdateFilterListPrefs,
                     base::Unretained(this), id, it->second->GetInfo()));
}

void AdBlockSubscriptionServiceManager::DeleteSubscription(const SubscriptionIdentifier& id) {
  auto it = subscription_services_.find(id);
  DCHECK(it != subscription_services_.end());
  it->second->Unregister();
  subscription_services_.erase(it);

  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&AdBlockSubscriptionServiceManager::ClearFilterListPrefs,
                     base::Unretained(this), id));
}

void AdBlockSubscriptionServiceManager::RefreshSubscription(const SubscriptionIdentifier& id) {
  auto it = subscription_services_.find(id);
  DCHECK(it != subscription_services_.end());
  download_manager_->StartDownload(it->second->GetInfo().list_url, true);
}

void AdBlockSubscriptionServiceManager::RefreshAllSubscriptions() {
  for (const auto& subscription_service : subscription_services_) {
    download_manager_->StartDownload(subscription_service.second->GetInfo().list_url, false);
  }
}

void AdBlockSubscriptionServiceManager::InitializeDownloadManager(Profile* system_profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* profile_key = system_profile->GetProfileKey();

  download_manager_ =
        std::make_unique<CustomSubscriptionDownloadManager>(
            BraveDownloadServiceFactory::GetForKey(profile_key),
            base::ThreadPool::CreateSequencedTaskRunner(
                {base::MayBlock(), base::TaskPriority::BEST_EFFORT}));
}

void AdBlockSubscriptionServiceManager::StartSubscriptionServices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state)
    return;

  const base::DictionaryValue* list_subscriptions_dict =
      local_state->GetDictionary(kAdBlockListSubscriptions);
  if (!list_subscriptions_dict || list_subscriptions_dict->empty()) {
    return;
  }

  for (base::DictionaryValue::Iterator it(*list_subscriptions_dict);
       !it.IsAtEnd(); it.Advance()) {
    const std::string uuid = it.key();
    FilterListSubscriptionInfo info;
    const base::Value* list_subscription_dict = list_subscriptions_dict->FindDictKey(uuid);
    if (list_subscription_dict) {
      info = BuildInfoFromDict(GURL(uuid), list_subscription_dict);

      auto subscription_service = AdBlockSubscriptionServiceFactory(info, delegate_);
      subscription_service->Start();

      subscription_services_.insert(std::make_pair(uuid, std::move(subscription_service)));
    }
  }

  initialized_ = true;
}

// Updates preferences to reflect a new state for the specified filter list.
// Creates the entry if it does not yet exist.
void AdBlockSubscriptionServiceManager::UpdateFilterListPrefs(
    const SubscriptionIdentifier& id,
    const FilterListSubscriptionInfo& info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state) {
    return;
  }
  DictionaryPrefUpdate update(local_state, kAdBlockListSubscriptions);
  base::DictionaryValue* subscriptions_dict = update.Get();
  auto subscription_dict = base::Value(base::Value::Type::DICTIONARY);
  subscription_dict.SetBoolKey("enabled", info.enabled);
  subscription_dict.SetDoubleKey("last_update_attempt", info.last_update_attempt.ToJsTime());
  subscription_dict.SetBoolKey("last_update_was_successful", info.last_update_was_successful);
  subscriptions_dict->SetKey(id.spec(), std::move(subscription_dict));
}

// Updates preferences to remove all state for the specified filter list.
void AdBlockSubscriptionServiceManager::ClearFilterListPrefs(
    const SubscriptionIdentifier& id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state) {
    return;
  }
  DictionaryPrefUpdate update(local_state, kAdBlockListSubscriptions);
  base::DictionaryValue* subscriptions_dict = update.Get();
  subscriptions_dict->RemoveKey(id.spec());
}

bool AdBlockSubscriptionServiceManager::IsInitialized() const {
  return initialized_;
}

bool AdBlockSubscriptionServiceManager::Start() {
  //base::AutoLock lock(regional_services_lock_);
  for (const auto& subscription_service : subscription_services_) {
    subscription_service.second->Start();
  }
  StartSubscriptionServices();
  return true;
}

void AdBlockSubscriptionServiceManager::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url) {
  //base::AutoLock lock(regional_services_lock_);
  for (const auto& subscription_service : subscription_services_) {
    if (subscription_service.second->GetInfo().enabled) {
      subscription_service.second->ShouldStartRequest(
          url, resource_type, tab_host, did_match_rule, did_match_exception,
          did_match_important, mock_data_url);
      if (did_match_important && *did_match_important) {
        return;
      }
    }
  }
}

void AdBlockSubscriptionServiceManager::EnableTag(const std::string& tag,
                                              bool enabled) {
  //base::AutoLock lock(regional_services_lock_);
  for (const auto& subscription_service : subscription_services_) {
    subscription_service.second->EnableTag(tag, enabled);
  }
}

void AdBlockSubscriptionServiceManager::AddResources(
    const std::string& resources) {
  //base::AutoLock lock(regional_services_lock_);
  for (const auto& subscription_service : subscription_services_) {
    subscription_service.second->AddResources(resources);
  }
}

base::Optional<base::Value>
AdBlockSubscriptionServiceManager::UrlCosmeticResources(
        const std::string& url) {
  //base::AutoLock lock(regional_services_lock_);
  base::Optional<base::Value> first_value = base::nullopt;

  for (auto it = subscription_services_.begin(); it != subscription_services_.end(); it++) {
    if (it->second->GetInfo().enabled) {
      base::Optional<base::Value> next_value =
          it->second->UrlCosmeticResources(url);
      if (first_value) {
        if (next_value) {
          MergeResourcesInto(std::move(*next_value), &*first_value, false);
        }
      } else {
        first_value = std::move(next_value);
      }
    }
  }

  return first_value;
}

base::Optional<base::Value>
AdBlockSubscriptionServiceManager::HiddenClassIdSelectors(
        const std::vector<std::string>& classes,
        const std::vector<std::string>& ids,
        const std::vector<std::string>& exceptions) {
  //base::AutoLock lock(regional_services_lock_);
  base::Optional<base::Value> first_value = base::nullopt;

  for (auto it = subscription_services_.begin(); it != subscription_services_.end(); it++) {
    if (it->second->GetInfo().enabled) {
      base::Optional<base::Value> next_value =
          it->second->HiddenClassIdSelectors(classes, ids, exceptions);
      if (first_value && first_value->is_list()) {
        if (next_value && next_value->is_list()) {
          for (auto i = next_value->GetList().begin();
                  i < next_value->GetList().end();
                  i++) {
            first_value->Append(std::move(*i));
          }
        }
      } else {
        first_value = std::move(next_value);
      }
    }
  }

  return first_value;
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockSubscriptionServiceManager>
AdBlockSubscriptionServiceManagerFactory(BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockSubscriptionServiceManager>(delegate);
}

}  // namespace brave_shields
