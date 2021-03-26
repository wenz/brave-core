/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "content/public/browser/web_ui.h"

#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#include "../../../../../../../chrome/browser/ui/webui/settings/metrics_reporting_handler.cc"
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
