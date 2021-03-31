/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/custom_subscription_download_manager.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "components/download/public/background_service/download_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "storage/browser/blob/blob_data_handle.h"

namespace brave_shields {

namespace {

const net::NetworkTrafficAnnotationTag
    kBraveShieldsCustomSubscriptionTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation("brave_shields_custom_subscription",
                                            R"(
        semantics {
          sender: "Brave Shields"
          description:
            "Brave periodically downloads updates to third-party filter lists "
            "added by users on brave://adblock."
          trigger:
            "After being registered in brave://adblock, any enabled filter "
            "list subscriptions will be updated in accordance with their "
            "`Expires` field if present, or daily otherwise. A manual refresh "
            "for a particular list can also be triggered in brave://adblock."
          data: "The URL endpoint provided by the user in brave://adblock to "
            "fetch list updates from. No user information is sent."
          destination: BRAVE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled in settings. However it will "
            "never be made if the corresponding entry is removed from the "
            "brave://adblock page's custom list subscription section."
          policy_exception_justification: "Not yet implemented."
        })");

}  // namespace

CustomSubscriptionDownloadManager::CustomSubscriptionDownloadManager(
    download::DownloadService* download_service,
    scoped_refptr<base::SequencedTaskRunner> background_task_runner)
    : download_service_(download_service),
      is_available_for_downloads_(true),
      background_task_runner_(background_task_runner) {}

CustomSubscriptionDownloadManager::~CustomSubscriptionDownloadManager() = default;

void CustomSubscriptionDownloadManager::StartDownload(const GURL& download_url, bool high_priority) {
  download::DownloadParams download_params;
  download_params.client =
      download::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS;
  download_params.guid = base::GenerateGUID();
  download_params.callback =
      base::BindRepeating(&CustomSubscriptionDownloadManager::OnDownloadStarted,
                          ui_weak_ptr_factory_.GetWeakPtr(),
                          download_url);
  download_params.traffic_annotation = net::MutableNetworkTrafficAnnotationTag(
      kBraveShieldsCustomSubscriptionTrafficAnnotation);
  download_params.request_params.url = download_url;
  download_params.request_params.method = "GET";
  if (high_priority) {
    // This triggers a high priority download with no network restrictions to
    // provide status feedback as quickly as possible.
    download_params.scheduling_params.priority =
        download::SchedulingParams::Priority::HIGH;
    download_params.scheduling_params.battery_requirements =
        download::SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
    download_params.scheduling_params.network_requirements =
        download::SchedulingParams::NetworkRequirements::NONE;
  } else {
    download_params.scheduling_params.priority =
        download::SchedulingParams::Priority::NORMAL;
    download_params.scheduling_params.battery_requirements =
        download::SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
    download_params.scheduling_params.network_requirements =
        download::SchedulingParams::NetworkRequirements::OPTIMISTIC;
  }

  download_service_->StartDownload(download_params);
}

void CustomSubscriptionDownloadManager::CancelAllPendingDownloads() {
  for (const std::pair<std::string, GURL> pending_download : pending_download_guids_) {
    const std::string& pending_download_guid = pending_download.first;
    download_service_->CancelDownload(pending_download_guid);
  }
}

bool CustomSubscriptionDownloadManager::IsAvailableForDownloads() const {
  return is_available_for_downloads_;
}

void CustomSubscriptionDownloadManager::OnDownloadServiceReady(
    const std::set<std::string>& pending_download_guids,
    const std::map<std::string, base::FilePath>& successful_downloads) {
  for (const std::string& pending_download_guid : pending_download_guids) {
    LOG(ERROR) << pending_download_guid;
    DCHECK(false);  // TODO
    //pending_download_guids_.insert(pending_download_guid);
  }

  // Successful downloads should already be notified via |onDownloadSucceeded|,
  // so we don't do anything with them here.
}

void CustomSubscriptionDownloadManager::OnDownloadServiceUnavailable() {
  is_available_for_downloads_ = false;
}

void CustomSubscriptionDownloadManager::OnDownloadStarted(
    const GURL download_url,
    const std::string& guid,
    download::DownloadParams::StartResult start_result) {
  if (start_result == download::DownloadParams::StartResult::ACCEPTED) {
    pending_download_guids_.insert(std::pair<std::string, GURL>(guid, download_url));
  }
}

void CustomSubscriptionDownloadManager::OnDownloadSucceeded(
    const std::string& guid,
    std::unique_ptr<storage::BlobDataHandle> data_handle) {
  GURL download_url;
  auto it = pending_download_guids_.find(guid);
  if (it == pending_download_guids_.end()) {
    // TODO fail gracefully (cleanup the file), or turn this into a proper assert
    DCHECK(false);
  } else {
    download_url = it->second;
  }
  pending_download_guids_.erase(guid);

  base::UmaHistogramBoolean(
      "BraveShields.CustomSubscriptionDownloadManager.DownloadSucceeded",
      true);

}

void CustomSubscriptionDownloadManager::OnDownloadFailed(const std::string& guid) {
  pending_download_guids_.erase(guid);

  base::UmaHistogramBoolean(
      "BraveShields.CustomSubscriptionDownloadManager.DownloadSucceeded",
      false);
}


}  // namespace brave_shields
