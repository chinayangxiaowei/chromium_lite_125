// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_sharing/public/features.h"

#include "base/feature_list.h"

namespace data_sharing::features {

BASE_FEATURE(kDataSharingFeature,
             "DataSharing",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace data_sharing::features
