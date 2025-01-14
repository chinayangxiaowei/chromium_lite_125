// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_backend_factory.h"

#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/bind_post_task.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/password_manager/core/browser/password_manager_buildflags.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_store.h"
#include "components/password_manager/core/browser/password_store/password_store_backend.h"
#include "components/password_manager/core/browser/password_store/password_store_built_in_backend.h"
#include "components/password_manager/core/browser/password_store_factory_util.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"

#if !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
#include "chrome/browser/password_manager/android/android_backend_with_double_deletion.h"
#include "chrome/browser/password_manager/android/legacy_password_store_backend_migration_decorator.h"
#include "chrome/browser/password_manager/android/password_manager_eviction_util.h"
#include "chrome/browser/password_manager/android/password_store_android_account_backend.h"
#include "chrome/browser/password_manager/android/password_store_android_local_backend.h"
#include "chrome/browser/password_manager/android/password_store_backend_migration_decorator.h"
#include "chrome/browser/password_manager/android/password_store_proxy_backend.h"
#include "components/password_manager/core/browser/features/password_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#endif  // !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/password_manager/android/password_manager_android_util.h"
#include "chrome/browser/password_manager/android/password_store_proxy_backend.h"
#endif

namespace {

#if !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
using password_manager::prefs::UseUpmLocalAndSeparateStoresState;

std::unique_ptr<password_manager::PasswordStoreBackend>
CreateProfilePasswordStoreBackendForUpmAndroid(
    PrefService* prefs,
    std::unique_ptr<password_manager::PasswordStoreBuiltInBackend>
        built_in_backend,
    password_manager::AffiliationsPrefetcher* affiliations_prefetcher) {
  base::UmaHistogramBoolean(
      "PasswordManager.PasswordStore.WasEnrolledInUPMWhenBackendWasCreated",
      !prefs->GetBoolean(password_manager::prefs::
                             kUnenrolledFromGoogleMobileServicesDueToErrors));
  base::UmaHistogramCounts100(
      "PasswordManager.PasswordStore.TimesReenrolledInUPM",
      prefs->GetInteger(
          password_manager::prefs::kTimesReenrolledToGoogleMobileServices));
  base::UmaHistogramCounts100(
      "PasswordManager.PasswordStore.TimesAttemptedToReenrollInUPM",
      prefs->GetInteger(password_manager::prefs::
                            kTimesAttemptedToReenrollToGoogleMobileServices));
  auto useSplitStores =
      static_cast<UseUpmLocalAndSeparateStoresState>(prefs->GetInteger(
          password_manager::prefs::kPasswordsUseUPMLocalAndSeparateStores));
  // Creates the password store backend for the profile store on Android
  // platform when UPM is enabled. There are 3 cases:
  switch (useSplitStores) {
    // UPM M3: The password store migration decorator is created as backend. It
    // is expected to migrate the passwords from the built in profile store to
    // the GMS core local store.
    case UseUpmLocalAndSeparateStoresState::kOffAndMigrationPending:
      return std::make_unique<
          password_manager::PasswordStoreBackendMigrationDecorator>(
          std::move(built_in_backend),
          std::make_unique<password_manager::PasswordStoreAndroidLocalBackend>(
              prefs, affiliations_prefetcher),
          prefs);
    // UPM M2: The password store proxy backend is created. No migrations are
    // needed.
    case UseUpmLocalAndSeparateStoresState::kOn:
      return std::make_unique<AndroidBackendWithDoubleDeletion>(
          std::move(built_in_backend),
          std::make_unique<password_manager::PasswordStoreAndroidLocalBackend>(
              prefs, affiliations_prefetcher));
    // Old UPM: support for local passwords in GMSCore is unavailable for some
    // reason.
    case UseUpmLocalAndSeparateStoresState::kOff: {
      // Even though this is a backend for a ProfilePasswordStore it has to
      // talk to the account. Before the store split, the `ProfilePasswordStore`
      // only supports talking to the account storage in GMS Core. All local
      // storage requests go to the built-in backend instead.
      auto android_account_backend = std::make_unique<
          password_manager::PasswordStoreAndroidAccountBackend>(
          prefs, affiliations_prefetcher, password_manager::kProfileStore);
      if (base::FeatureList::IsEnabled(
              password_manager::features::
                  kUnifiedPasswordManagerSyncOnlyInGMSCore)) {
        // M4 feature flag is enabled. Chrome stops trying to migrate passwords
        // to the account GMSCore storage. Only PasswordStoreProxyBackend is
        // created.
        return std::make_unique<password_manager::PasswordStoreProxyBackend>(
            std::move(built_in_backend), std::move(android_account_backend),
            prefs);
      }
      // The password store migration decorator is created as backend.
      // There are no split stores at this stage, and the decorator is expected
      // to migrate the passwords from the built in profile store to the GMS
      // core account store.
      return std::make_unique<
          password_manager::LegacyPasswordStoreBackendMigrationDecorator>(
          std::move(built_in_backend), std::move(android_account_backend),
          prefs);
    }
  }
}
#endif  // !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
}  // namespace

std::unique_ptr<password_manager::PasswordStoreBackend>
CreateProfilePasswordStoreBackend(
    const base::FilePath& login_db_directory,
    PrefService* prefs,
    password_manager::AffiliationsPrefetcher* affiliations_prefetcher) {
  TRACE_EVENT0("passwords", "PasswordStoreBackendCreation");

  auto is_profile_db_empty_cb =
#if BUILDFLAG(IS_ANDROID)
      // base::Unretained() is safe, `prefs` outlives all keyed services,
      // including the PasswordStore (LoginDatabase's owner).
      base::BindPostTaskToCurrentDefault(base::BindRepeating(
          &PrefService::SetBoolean, base::Unretained(prefs),
          password_manager::prefs::kEmptyProfileStoreLoginDatabase));
#else
      base::NullCallback();
#endif

#if BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  return std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
      password_manager::CreateLoginDatabaseForProfileStorage(
          login_db_directory, std::move(is_profile_db_empty_cb)),
      syncer::WipeModelUponSyncDisabledBehavior::kNever, prefs);
#else  // BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  std::unique_ptr<password_manager::LoginDatabase> profile_login_db =
      password_manager::CreateLoginDatabaseForProfileStorage(
          login_db_directory, std::move(is_profile_db_empty_cb));
  auto built_in_backend =
      std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
          std::move(profile_login_db),
          syncer::WipeModelUponSyncDisabledBehavior::kNever, prefs);

  // This are the absolute minimum requirements to have any version of UPM.
  if (password_manager_android_util::AreMinUpmRequirementsMet()) {
    return CreateProfilePasswordStoreBackendForUpmAndroid(
        prefs, std::move(built_in_backend), affiliations_prefetcher);
  }
  return built_in_backend;
#endif
}

std::unique_ptr<password_manager::PasswordStoreBackend>
CreateAccountPasswordStoreBackend(
    const base::FilePath& login_db_directory,
    PrefService* prefs,
    std::unique_ptr<password_manager::UnsyncedCredentialsDeletionNotifier>
        unsynced_deletions_notifier,
    password_manager::AffiliationsPrefetcher* affiliations_prefetcher) {
  std::unique_ptr<password_manager::LoginDatabase> login_db(
      password_manager::CreateLoginDatabaseForAccountStorage(
          login_db_directory));
#if BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  return std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
      std::move(login_db), syncer::WipeModelUponSyncDisabledBehavior::kAlways,
      prefs, std::move(unsynced_deletions_notifier));
#else  // BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  if (!password_manager_android_util::AreMinUpmRequirementsMet()) {
    // Can happen if the downstream code is not available.
    return std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
        std::move(login_db), syncer::WipeModelUponSyncDisabledBehavior::kAlways,
        prefs);
  }

  // Note: The built-in backend is backed by the login database and Chrome
  // syncs it. As such, it expects local data to be cleared every time when
  // sync is permanently disabled and thus uses
  // WipeModelUponSyncDisabledBehavior::kAlways.
  return std::make_unique<AndroidBackendWithDoubleDeletion>(
      std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
          std::move(login_db),
          syncer::WipeModelUponSyncDisabledBehavior::kAlways, prefs),
      std::make_unique<password_manager::PasswordStoreAndroidAccountBackend>(
          prefs, affiliations_prefetcher, password_manager::kAccountStore));
#endif
}
