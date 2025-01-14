// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/docking_promo/model/utils.h"

#import "base/feature_list.h"
#import "base/metrics/histogram_macros.h"
#import "base/strings/sys_string_conversions.h"
#import "base/time/time.h"
#import "ios/chrome/browser/default_browser/model/utils.h"
#import "ios/chrome/browser/promos_manager/model/constants.h"
#import "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#import "ios/chrome/browser/shared/model/utils/first_run_util.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/public/features/system_flags.h"
#import "ios/chrome/browser/ui/start_surface/start_surface_util.h"

namespace {

// Killswitch to control new DockingPromo histograms.
BASE_FEATURE(kDockingPromoHistogramKillswitch,
             "DockingPromoHistogramKillswitch",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace

BOOL IsDockingPromoForcedForDisplay() {
  NSString* forced_promo_name = experimental_flags::GetForcedPromoToDisplay();

  if ([forced_promo_name length] > 0) {
    std::optional<promos_manager::Promo> forced_promo =
        promos_manager::PromoForName(
            base::SysNSStringToUTF8(forced_promo_name));

    if (!forced_promo.has_value()) {
      return NO;
    }

    return forced_promo.value() == promos_manager::Promo::DockingPromo;
  }

  return NO;
}

BOOL CanShowDockingPromo(base::TimeDelta time_since_last_foreground) {
  // TODO(crbug.com/330387623): Cleanup Docking Promo histograms.
  if (base::FeatureList::IsEnabled(kDockingPromoHistogramKillswitch)) {
    // Logs the time since last foreground over a range of 2 weeks.
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "IOS.DockingPromo.LastForegroundTimeViaAppState",
        time_since_last_foreground.InMinutes(), 1, base::Days(14).InMinutes(),
        100);
  }

  if (IsDockingPromoForcedForDisplay()) {
    return YES;
  }

  if (IsChromeLikelyDefaultBrowser()) {
    return NO;
  }

  // For users no older than 2 days, whether they're active on their first day,
  // but not their second day.
  BOOL second_day_inactive =
      IsFirstRunRecent(base::Days(2) + base::Seconds(1)) &&
      (time_since_last_foreground >
       base::Hours(HoursInactiveForNewUsersUntilShowingDockingPromo()));

  // For users no older than 14 days, whether they've been inactive for 3
  // consecutive (or more) days.
  BOOL three_or_more_days_inactive =
      IsFirstRunRecent(base::Days(14) + base::Seconds(1)) &&
      (time_since_last_foreground >
       base::Hours(HoursInactiveForOldUsersUntilShowingDockingPromo()));

  return second_day_inactive || three_or_more_days_inactive;
}

std::optional<base::TimeDelta> MinTimeSinceLastForeground(
    NSArray<SceneState*>* foregroundScenes) {
  std::optional<base::TimeDelta> minTimeSinceLastForeground = std::nullopt;

  for (SceneState* scene in foregroundScenes) {
    const base::TimeDelta timeSinceLastForeground =
        GetTimeSinceMostRecentTabWasOpenForSceneState(scene);

    if (!minTimeSinceLastForeground.has_value()) {
      minTimeSinceLastForeground = timeSinceLastForeground;
    } else if (timeSinceLastForeground < minTimeSinceLastForeground.value()) {
      minTimeSinceLastForeground = timeSinceLastForeground;
    }
  }

  return minTimeSinceLastForeground;
}
