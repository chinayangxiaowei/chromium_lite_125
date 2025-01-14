// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_RESTORE_WEB_CONTENTS_MANAGER_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_RESTORE_WEB_CONTENTS_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "base/types/pass_key.h"

class Profile;

namespace content {
class WebContents;
}

namespace webapps {
class WebAppUrlLoader;
}

namespace webapk {

class WebApkRestoreManager;

// This manager is intended to wrap all of the functionality that the
// WebApk restores needs from `WebContents` to allow easy unit testing.
// This encompasses retrieving any information from a given `WebContents`.
//
// TODO(b/280517254): Have this class more fully encompass the WebContents
// dependency, instead of creating classes that operate on it.
class WebApkRestoreWebContentsManager {
 public:
  explicit WebApkRestoreWebContentsManager(Profile* profile);
  WebApkRestoreWebContentsManager(const WebApkRestoreWebContentsManager&) =
      delete;
  WebApkRestoreWebContentsManager& operator=(
      const WebApkRestoreWebContentsManager&) = delete;
  ~WebApkRestoreWebContentsManager();

  void EnsureWebContentsCreated(base::PassKey<WebApkRestoreManager> pass_key);
  void ClearSharedWebContents();

  std::unique_ptr<webapps::WebAppUrlLoader> CreateUrlLoader();

  base::WeakPtr<WebApkRestoreWebContentsManager> GetWeakPtr();

  content::WebContents* web_contents() { return shared_web_contents_.get(); }

 private:
  const base::WeakPtr<Profile> profile_;

  // WebContents are shared between tasks to avoid creating it multiple time. It
  // should be reset when task queue is empty.
  std::unique_ptr<content::WebContents> shared_web_contents_;

  base::WeakPtrFactory<WebApkRestoreWebContentsManager> weak_ptr_factory_{this};
};

}  // namespace webapk

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_RESTORE_WEB_CONTENTS_MANAGER_H_
