// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header contains field trial and variations definitions for policies,
// mechanisms and features in the performance_manager component.

#ifndef COMPONENTS_PERFORMANCE_MANAGER_PUBLIC_FEATURES_H_
#define COMPONENTS_PERFORMANCE_MANAGER_PUBLIC_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace performance_manager::features {

// If enabled the PM runs on the main (UI) thread. Cannot be enabled
// simultaneously with `kRunOnMainThreadSync`.
BASE_DECLARE_FEATURE(kRunOnMainThread);

// If enabled, the PM runs on the main (UI) thread *and* tasks posted to the PM
// TaskRunner from the main (UI) thread run synchronously. Cannot be enabled
// simultaneously with `kRunOnMainThread`. This is a standalone feature rather
// than a param on `kRunOnMainThreadSync` because accessing the state of a
// `base::Feature` is faster than accessing the state of a `base::FeatureParam`.
BASE_DECLARE_FEATURE(kRunOnMainThreadSync);

#if !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_LINUX)
#define URGENT_DISCARDING_FROM_PERFORMANCE_MANAGER() false
#else
#define URGENT_DISCARDING_FROM_PERFORMANCE_MANAGER() true
#endif

// Enables urgent discarding of pages directly from PerformanceManager rather
// than via TabManager on Ash Chrome.
#if BUILDFLAG(IS_CHROMEOS_ASH)
BASE_DECLARE_FEATURE(kAshUrgentDiscardingFromPerformanceManager);
#endif

// When enabled removes the rate limit on reporting tab processes to resourced.
#if BUILDFLAG(IS_CHROMEOS)
BASE_DECLARE_FEATURE(kUnthrottledTabProcessReporting);
#endif

// Enable background tab loading of pages (restored via session restore)
// directly from Performance Manager rather than via TabLoader.
BASE_DECLARE_FEATURE(kBackgroundTabLoadingFromPerformanceManager);

// Make the Battery Saver Modes available to users. If this is enabled, it
// doesn't mean the mode is enabled, just that the user has the option of
// toggling it.
BASE_DECLARE_FEATURE(kBatterySaverModeAvailable);

// If enabled, makes battery saver request render process tuning.
BASE_DECLARE_FEATURE(kBatterySaverModeRenderTuning);

// Flag to control a baseline HaTS survey for Chrome performance.
BASE_DECLARE_FEATURE(kPerformanceControlsPerformanceSurvey);
BASE_DECLARE_FEATURE(kPerformanceControlsBatteryPerformanceSurvey);
BASE_DECLARE_FEATURE(kPerformanceControlsMemorySaverOptOutSurvey);
BASE_DECLARE_FEATURE(kPerformanceControlsBatterySaverOptOutSurvey);

// Defines the time delta to look back when checking if a device has used
// battery.
extern const base::FeatureParam<base::TimeDelta>
    kPerformanceControlsBatterySurveyLookback;

// Round 2 Performance Controls features

// This enables the UI for the multi-state version of memory saver mode.
BASE_DECLARE_FEATURE(kMemorySaverMultistateMode);
// When true, a recommended badge will be shown next to the heuristic memory
// saver option.
extern const base::FeatureParam<bool> kMemorySaverShowRecommendedBadge;

// Round 3 Performance Controls features

// This enables the performance controls side panel for learning about and
// configuring performance settings.
BASE_DECLARE_FEATURE(kPerformanceControlsSidePanel);

// This enables the CPU performance interventions within the side panel.
BASE_DECLARE_FEATURE(kPerformanceCPUIntervention);

#if BUILDFLAG(IS_WIN)
// Prefetch the main browser DLL when a new node is added to the PM graph
// and no prefetch has been done within a reasonable timeframe.
BASE_DECLARE_FEATURE(kPrefetchVirtualMemoryPolicy);
#endif

// This represents the duration that CPU must be over the threshold before
// a notification is triggered.
extern const base::FeatureParam<base::TimeDelta> kCPUTimeOverThreshold;

// Frequency to sample for cpu usage to ensure that the user is experiencing
// consistent cpu issues before surfacing a notification
extern const base::FeatureParam<base::TimeDelta> kCPUSampleFrequency;

// If the system CPU consistently exceeds these percent thresholds, then
// the CPU health will be classified as the threshold it is exceeding
extern const base::FeatureParam<int> kCPUDegradedHealthPercentageThreshold;
extern const base::FeatureParam<int> kCPUUnhealthyPercentageThreshold;

// Maximum number of tabs to be actionable
extern const base::FeatureParam<int> kCPUMaxActionableTabs;

// Minimum percentage to improve CPU health for a tab to be actionable
extern const base::FeatureParam<int> kMinimumActionableTabCPUPercentage;

// This enables the Memory performance interventions within the side panel.
BASE_DECLARE_FEATURE(kPerformanceMemoryIntervention);

// This represents the duration that Memory must be over the threshold before
// a notification is triggered.
extern const base::FeatureParam<base::TimeDelta> kMemoryTimeOverThreshold;

// If available Memory percent and bytes are both under the specified thresholds
// then we will trigger a notification.
extern const base::FeatureParam<int> kMemoryFreePercentThreshold;
extern const base::FeatureParam<int> kMemoryFreeBytesThreshold;

#endif

BASE_DECLARE_FEATURE(kPMProcessPriorityPolicy);

extern const base::FeatureParam<bool> kDownvoteAdFrames;

// When enabled, Memory Saver supports the different modes defined in the
// `ModalMemorySaverMode` enum.
BASE_DECLARE_FEATURE(kModalMemorySaver);

// When set, makes Memory Saver behave as the specified mode if it's  enabled.
extern const base::FeatureParam<int> kModalMemorySaverMode;

// Policy that evicts the BFCache of pages that become non visible or the
// BFCache of all pages when the system is under memory pressure.
BASE_DECLARE_FEATURE(kBFCachePerformanceManagerPolicy);

// Whether tabs are discarded under high memory pressure.
BASE_DECLARE_FEATURE(kUrgentPageDiscarding);

// This enables logging to evaluate the efficacy of potential CPU interventions.
BASE_DECLARE_FEATURE(kCPUInterventionEvaluationLogging);

// This represents the duration that CPU must be over the threshold before
// logging the delayed metrics.
extern const base::FeatureParam<base::TimeDelta> kDelayBeforeLogging;

// If Chrome CPU utilization is over the specified percent then we will log it.
extern const base::FeatureParam<int> kThresholdChromeCPUPercent;

// When enabled, the PageResource2 UKM is logged twice, once using Resource
// Attribution and once using legacy measurements, to compare the results.
BASE_DECLARE_FEATURE(kResourceAttributionValidation);

// When enabled, Resource Attribution measurements will include contexts for
// individual origins.
BASE_DECLARE_FEATURE(kResourceAttributionIncludeOrigins);

}  // namespace performance_manager::features

#endif  // COMPONENTS_PERFORMANCE_MANAGER_PUBLIC_FEATURES_H_
