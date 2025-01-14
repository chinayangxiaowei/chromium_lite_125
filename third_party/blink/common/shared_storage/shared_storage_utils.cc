// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/shared_storage/shared_storage_utils.h"

#include "base/metrics/histogram_functions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "third_party/blink/public/common/features.h"

namespace blink {

namespace {

size_t MaxChar16StringLength() {
  // Each char16_t takes 2 bytes.
  return static_cast<size_t>(features::kMaxSharedStorageBytesPerOrigin.Get()) /
         2;
}

}  // namespace

bool IsValidSharedStorageURLsArrayLength(size_t length) {
  return length > 0u &&
         length <=
             static_cast<size_t>(
                 features::kSharedStorageURLSelectionOperationInputURLSizeLimit
                     .Get());
}

bool IsValidSharedStorageKeyStringLength(size_t length) {
  return length > 0u && length <= MaxChar16StringLength();
}

bool IsValidSharedStorageValueStringLength(size_t length) {
  return length <= MaxChar16StringLength();
}

void LogSharedStorageWorkletError(SharedStorageWorkletErrorType error_type) {
  base::UmaHistogramEnumeration("Storage.SharedStorage.Worklet.Error.Type",
                                error_type);
}

bool ShouldDefinePrivateAggregationInSharedStorage() {
  return base::FeatureList::IsEnabled(
             blink::features::kPrivateAggregationApi) &&
         blink::features::kPrivateAggregationApiEnabledInSharedStorage.Get();
}

bool IsValidPrivateAggregationContextId(base::StringPiece context_id) {
  return context_id.size() <= blink::kPrivateAggregationApiContextIdMaxLength &&
         base::IsStringUTF8AllowingNoncharacters(context_id);
}

}  // namespace blink
