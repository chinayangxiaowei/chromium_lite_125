// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include "base/android/jni_array.h"
#include "base/android/scoped_java_ref.h"
#include "base/feature_list.h"
#include "chrome/browser/android/webapk/webapk_sync_service.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/sync/base/features.h"
#include "components/webapps/browser/android/webapps_jni_headers/PwaRestoreBottomSheetMediator_jni.h"

using base::android::JavaParamRef;

namespace webapk {

static void JNI_PwaRestoreBottomSheetMediator_OnRestoreWebapps(
    JNIEnv* env,
    const JavaParamRef<jobjectArray>& jrestore_app_ids) {
  if (!base::FeatureList::IsEnabled(chrome::android::kPwaRestoreUi)) {
    return;
  }

  Profile* profile = ProfileManager::GetLastUsedProfile();
  if (profile == nullptr) {
    return;
  }

  std::vector<std::string> app_ids_to_restore;
  base::android::AppendJavaStringArrayToStringVector(env, jrestore_app_ids,
                                                     &app_ids_to_restore);
  WebApkSyncService::GetForProfile(profile)->RestoreAppList(app_ids_to_restore);
}

}  // namespace webapk
