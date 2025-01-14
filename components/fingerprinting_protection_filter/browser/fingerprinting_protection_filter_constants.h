// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FINGERPRINTING_PROTECTION_FILTER_BROWSER_FINGERPRINTING_PROTECTION_FILTER_CONSTANTS_H_
#define COMPONENTS_FINGERPRINTING_PROTECTION_FILTER_BROWSER_FINGERPRINTING_PROTECTION_FILTER_CONSTANTS_H_

#include "base/files/file_path.h"
#include "components/subresource_filter/core/browser/ruleset_config.h"

namespace fingerprinting_protection_filter {

// The config used to identify the Fingerprinting Protection ruleset for the
// RulesetService. Encompasses a ruleset tag and top level directory name where
// the ruleset should be stored.
extern const subresource_filter::RulesetConfig
    kFingerprintingProtectionRulesetConfig;

}  // namespace fingerprinting_protection_filter

#endif  // COMPONENTS_FINGERPRINTING_PROTECTION_FILTER_BROWSER_FINGERPRINTING_PROTECTION_FILTER_CONSTANTS_H_
