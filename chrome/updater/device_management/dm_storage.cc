// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/updater/device_management/dm_storage.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/updater/device_management/dm_cached_policy_info.h"
#include "chrome/updater/device_management/dm_message.h"
#include "chrome/updater/protos/omaha_settings.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace updater {

namespace {

// This DM Token value is persisted if the server asks the client to invalidate
// the DM Token.
constexpr char kInvalidTokenValue[] = "INVALID_DM_TOKEN";

// This is the standard name for the file that PersistPolicies() uses to store
// a PolicyFetchResponse received from the DMServer during the previous request.
// The data within the PolicyFetchResponse, such as the public key, version, and
// timestamp are used for subsequent requests and validations of DMServer
// responses.
constexpr char kPolicyInfoFileName[] = "CachedPolicyInfo";

// This is the standard name for the file that PersistPolicies() uses for each
// {policy_type} that it receives from the DMServer.
constexpr char kPolicyFileName[] = "PolicyFetchResponse";

// Deletes the child directories in the cache root if they do not appear in
// `exclusion_set`.
bool DeletePolicies(const base::FilePath& cache_root,
                    const std::set<std::string>& exclusion_set) {
  bool result = true;
  base::FileEnumerator(cache_root,
                       /* recursive */ false, base::FileEnumerator::DIRECTORIES,
                       FILE_PATH_LITERAL("*"))
      .ForEach([&exclusion_set, &result](const base::FilePath& file) {
        if (exclusion_set.count(file.BaseName().MaybeAsASCII())) {
          return;
        }

        if (!base::DeletePathRecursively(file)) {
          result = false;
        }
      });

  return result;
}

bool MakePathGlobalAccessible(const base::FilePath& path) {
#if BUILDFLAG(IS_POSIX)
  return base::SetPosixFilePermissions(
      path, base::FILE_PERMISSION_USER_MASK |
                base::FILE_PERMISSION_READ_BY_GROUP |
                base::FILE_PERMISSION_EXECUTE_BY_GROUP |
                base::FILE_PERMISSION_READ_BY_OTHERS |
                base::FILE_PERMISSION_EXECUTE_BY_OTHERS);
#else
  // Use system default permission for that path.
  return true;
#endif  // BUILDFLAG(IS_POSIX)
}

}  // namespace

bool CreateGlobalAccessibleDirectory(const base::FilePath& path) {
  return base::CreateDirectory(path) && MakePathGlobalAccessible(path);
}

bool WriteContentToGlobalReadableFile(const base::FilePath& path,
                                      const std::string& content) {
  return base::ImportantFileWriter::WriteFileAtomically(path, content) &&
         MakePathGlobalAccessible(path);
}

DMStorage::DMStorage(const base::FilePath& policy_cache_root,
                     std::unique_ptr<TokenServiceInterface> token_service)
    : policy_cache_root_(policy_cache_root),
      policy_info_file_(policy_cache_root_.AppendASCII(kPolicyInfoFileName)),
      token_service_(std::move(token_service)) {
  CHECK(token_service_);
}

DMStorage::~DMStorage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool DMStorage::InvalidateDMToken() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;
  return token_service_->StoreDmToken(kInvalidTokenValue);
}

bool DMStorage::DeleteDMToken() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;
  return token_service_->DeleteDmToken();
}

bool DMStorage::IsValidDMToken() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string dm_token = GetDmToken();
  return !dm_token.empty() && dm_token != kInvalidTokenValue;
}

bool DMStorage::IsDeviceDeregistered() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetDmToken() == kInvalidTokenValue;
}

bool DMStorage::CanPersistPolicies() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return base::PathExists(policy_info_file_)
             ? base::PathIsWritable(policy_info_file_)
             : base::ScopedTempDir().CreateUniqueTempDirUnderPath(
                   policy_cache_root_) &&
                   MakePathGlobalAccessible(policy_cache_root_);
}

bool DMStorage::PersistPolicies(const DMPolicyMap& policy_map) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (policy_map.empty()) {
    return true;
  }

  // Persists individual policies.
  std::set<std::string> updated_policy_set;
  bool policy_info_data_saved = false;
  for (const auto& policy_entry : policy_map) {
    const std::string& policy_type = policy_entry.first;
    const std::string& policy_value = policy_entry.second;
    if (!policy_info_data_saved) {
      // Policy info has a new public key when server rotates the key.
      // In this case, persists the policy info as the cached policy info
      // for future policy fetch.
      CachedPolicyInfo cached_info;
      if (cached_info.Populate(policy_value) &&
          !cached_info.public_key().empty() &&
          WriteContentToGlobalReadableFile(policy_info_file_, policy_value)) {
        VLOG(1) << "Public key rotated.";
        policy_info_data_saved = true;
      }
    }

    std::string encoded_policy_type = base::Base64Encode(policy_type);
    updated_policy_set.emplace(encoded_policy_type);

    const base::FilePath policy_dir =
        policy_cache_root_.AppendASCII(encoded_policy_type);
    if (!CreateGlobalAccessibleDirectory(policy_dir) ||
        !WriteContentToGlobalReadableFile(
            policy_dir.AppendASCII(kPolicyFileName), policy_value)) {
      return false;
    }
  }

  // All policies not in `updated_policy_set` are considered stale and deleted.
  return DeletePolicies(policy_cache_root_, updated_policy_set);
}

bool DMStorage::RemoveAllPolicies() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << __func__;
  return base::DeleteFile(policy_info_file_) &&
         DeletePolicies(policy_cache_root_, /*exclusion_set=*/{});
}

std::unique_ptr<CachedPolicyInfo> DMStorage::GetCachedPolicyInfo() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto cached_info = std::make_unique<CachedPolicyInfo>();

  std::string policy_info_data;
  if (IsValidDMToken() && base::PathExists(policy_info_file_) &&
      base::ReadFileToString(policy_info_file_, &policy_info_data)) {
    cached_info->Populate(policy_info_data);
  }

  return cached_info;
}

std::unique_ptr<
    ::wireless_android_enterprise_devicemanagement::OmahaSettingsClientProto>
DMStorage::GetOmahaPolicySettings() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!IsValidDMToken()) {
    VLOG(1) << "No valid DM token.";
    return nullptr;
  }

  std::string encoded_omaha_policy_type =
      base::Base64Encode(kGoogleUpdatePolicyType);

  base::FilePath omaha_policy_file =
      policy_cache_root_.AppendASCII(encoded_omaha_policy_type)
          .AppendASCII(kPolicyFileName);
  std::string response_data;
  ::enterprise_management::PolicyFetchResponse response;
  ::enterprise_management::PolicyData policy_data;
  auto omaha_settings =
      std::make_unique<::wireless_android_enterprise_devicemanagement::
                           OmahaSettingsClientProto>();
  if (!base::PathExists(omaha_policy_file) ||
      !base::ReadFileToString(omaha_policy_file, &response_data) ||
      response_data.empty() || !response.ParseFromString(response_data) ||
      !policy_data.ParseFromString(response.policy_data()) ||
      !policy_data.has_policy_value() ||
      !omaha_settings->ParseFromString(policy_data.policy_value())) {
    VLOG(1) << "No Omaha policies.";
    return nullptr;
  }

  return omaha_settings;
}

}  // namespace updater
