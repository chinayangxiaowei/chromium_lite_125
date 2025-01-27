// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_LOCAL_USER_FILES_FILE_LOCATION_UTILS_H_
#define CHROME_BROWSER_ASH_POLICY_LOCAL_USER_FILES_FILE_LOCATION_UTILS_H_

#include <string>

#include "base/files/file_path.h"

namespace policy::local_user_files {

// Returns true if `str` is a valid location string with optional placeholders.
bool IsValidLocationString(const std::string& str);

// Resolves possible path placeholders for external storages in `path_str` that
// can be set by policies. Returns empty path if can't resolve.
base::FilePath ResolvePath(const std::string& path_str);

}  // namespace policy::local_user_files

#endif  // CHROME_BROWSER_ASH_POLICY_LOCAL_USER_FILES_FILE_LOCATION_UTILS_H_
