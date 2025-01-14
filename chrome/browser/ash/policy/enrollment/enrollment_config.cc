// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/policy/enrollment/enrollment_config.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/values.h"
#include "chrome/browser/ash/login/login_pref_names.h"
#include "chrome/browser/ash/login/ui/login_display_host.h"
#include "chrome/browser/ash/login/wizard_context.h"
#include "chrome/browser/ash/policy/core/device_cloud_policy_manager_ash.h"
#include "chrome/browser/ash/policy/server_backed_state/server_backed_device_state.h"
#include "chrome/browser/ash/settings/device_settings_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "chromeos/ash/components/install_attributes/install_attributes.h"
#include "chromeos/ash/components/system/statistics_provider.h"
#include "components/prefs/pref_service.h"

namespace {

const char kRecoveryHistogram[] = "EnterpriseCheck.EnrollementRecoveryOnBoot";

// Do not reorder or delete entries because it is used in UMA.
enum class EnrollmentRecoveryOnBootUma {
  kForced = 0,
  kFalseFlag = 1,
  kNoSerialNumber = 2,
  kMaxValue = kNoSerialNumber,
};

std::string GetString(const base::Value::Dict& dict, std::string_view key) {
  const std::string* value = dict.FindString(key);
  return value ? *value : std::string();
}

bool IsEnrollingAfterRollback() {
  auto* login_display_host = ash::LoginDisplayHost::default_host();
  if (!login_display_host)
    return false;
  const auto* wizard_context = login_display_host->GetWizardContext();
  return wizard_context && ash::IsRollbackFlow(*wizard_context);
}

// Returns the license type to use based on the license type, assigned
// upgrade type and the license packaged from device state.
policy::LicenseType GetLicenseTypeToUse(
    const std::string license_type, const bool is_license_packaged_with_device,
    const std::string assigned_upgrade_type) {
  if (license_type == policy::kDeviceStateLicenseTypeEnterprise) {
    return policy::LicenseType::kEnterprise;
  } else if (license_type == policy::kDeviceStateLicenseTypeEducation) {
    return policy::LicenseType::kEducation;
  } else if (license_type == policy::kDeviceStateLicenseTypeTerminal) {
    return policy::LicenseType::kTerminal;
  }

  if (!is_license_packaged_with_device &&
      assigned_upgrade_type == policy::kDeviceStateAssignedUpgradeTypeKiosk) {
    return policy::LicenseType::kTerminal;
  }

  return policy::LicenseType::kNone;
}

std::string_view ToStringView(policy::EnrollmentConfig::Mode mode) {
#define CASE(_name)                           \
  case policy::EnrollmentConfig::Mode::_name: \
    return #_name;

  switch (mode) {
    CASE(MODE_NONE);
    CASE(MODE_MANUAL);
    CASE(MODE_MANUAL_REENROLLMENT);
    CASE(MODE_LOCAL_FORCED);
    CASE(MODE_LOCAL_ADVERTISED);
    CASE(MODE_SERVER_FORCED);
    CASE(MODE_SERVER_ADVERTISED);
    CASE(MODE_RECOVERY);
    CASE(MODE_ATTESTATION);
    CASE(MODE_ATTESTATION_LOCAL_FORCED);
    CASE(MODE_ATTESTATION_SERVER_FORCED);
    CASE(MODE_ATTESTATION_MANUAL_FALLBACK);
    CASE(DEPRECATED_MODE_OFFLINE_DEMO);
    CASE(DEPRECATED_MODE_ENROLLED_ROLLBACK);
    CASE(MODE_INITIAL_SERVER_FORCED);
    CASE(MODE_ATTESTATION_INITIAL_SERVER_FORCED);
    CASE(MODE_ATTESTATION_INITIAL_MANUAL_FALLBACK);
    CASE(MODE_ATTESTATION_ROLLBACK_FORCED);
    CASE(MODE_ATTESTATION_ROLLBACK_MANUAL_FALLBACK);
  }

  NOTREACHED_NORETURN();
#undef CASE
}

std::string_view ToStringView(policy::EnrollmentConfig::AuthMechanism auth) {
#define CASE(_name)                                    \
  case policy::EnrollmentConfig::AuthMechanism::_name: \
    return #_name;

  switch (auth) {
    CASE(AUTH_MECHANISM_INTERACTIVE);
    CASE(AUTH_MECHANISM_ATTESTATION);
    CASE(AUTH_MECHANISM_BEST_AVAILABLE);
  }

  NOTREACHED_NORETURN();
#undef CASE
}

}  // namespace

namespace policy {

EnrollmentConfig::EnrollmentConfig() = default;
EnrollmentConfig::EnrollmentConfig(const EnrollmentConfig& other) = default;
EnrollmentConfig::~EnrollmentConfig() = default;

// static
EnrollmentConfig EnrollmentConfig::GetPrescribedEnrollmentConfig() {
  return GetPrescribedEnrollmentConfig(
      g_browser_process->local_state(), *ash::InstallAttributes::Get(),
      ash::system::StatisticsProvider::GetInstance());
}

// static
EnrollmentConfig EnrollmentConfig::GetPrescribedEnrollmentConfig(
    PrefService* local_state,
    const ash::InstallAttributes& install_attributes,
    ash::system::StatisticsProvider* statistics_provider) {
  DCHECK(statistics_provider);

  EnrollmentConfig config;

  // Authentication through the attestation mechanism is controlled by a
  // command line switch that either enables it or forces it (meaning that
  // interactive authentication is disabled).
  switch (DeviceCloudPolicyManagerAsh::GetZeroTouchEnrollmentMode()) {
    case ZeroTouchEnrollmentMode::DISABLED:
      // Only use interactive authentication.
      config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_INTERACTIVE;
      break;

    case ZeroTouchEnrollmentMode::ENABLED:
      // Use the best mechanism, which may include attestation if available.
      config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_BEST_AVAILABLE;
      break;

    case ZeroTouchEnrollmentMode::FORCED:
      // Only use attestation to authenticate since zero-touch is forced.
      config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_ATTESTATION;
      break;
  }

  // If OOBE is done and we are not enrolled, make sure we only try interactive
  // enrollment.
  const bool oobe_complete = local_state->GetBoolean(ash::prefs::kOobeComplete);
  if (oobe_complete &&
      config.auth_mechanism == EnrollmentConfig::AUTH_MECHANISM_BEST_AVAILABLE)
    config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_INTERACTIVE;
  // If OOBE is done and we are enrolled, check for need to recover enrollment.
  // Enrollment recovery is not implemented for Active Directory.
  if (oobe_complete && install_attributes.IsCloudManaged()) {
    // Regardless what mode is applicable, the enrollment domain is fixed.
    config.management_domain = install_attributes.GetDomain();

    // Enrollment has completed previously and installation-time attributes
    // are in place. Enrollment recovery is required when the server
    // registration gets lost.
    if (local_state->GetBoolean(prefs::kEnrollmentRecoveryRequired)) {
      if (ash::DeviceSettingsService::IsInitialized() &&
          ash::DeviceSettingsService::Get()->HasDmToken()) {
        LOG(WARNING) << "False recovery flag.";
        local_state->ClearPref(::prefs::kEnrollmentRecoveryRequired);
        base::UmaHistogramEnumeration(kRecoveryHistogram,
                                      EnrollmentRecoveryOnBootUma::kFalseFlag);
      } else {
        LOG(WARNING) << "Enrollment recovery required according to pref.";
        const auto serial_number = statistics_provider->GetMachineID();
        if (!serial_number || serial_number->empty()) {
          LOG(WARNING) << "Postponing recovery because machine id is missing.";
          base::UmaHistogramEnumeration(
              kRecoveryHistogram, EnrollmentRecoveryOnBootUma::kNoSerialNumber);
        } else {
          config.mode = EnrollmentConfig::MODE_RECOVERY;
          base::UmaHistogramEnumeration(kRecoveryHistogram,
                                        EnrollmentRecoveryOnBootUma::kForced);
        }
      }
    }

    return config;
  }

  // OOBE is still running, or it is complete but the device hasn't been
  // enrolled yet. In either case, enrollment should take place if there's a
  // signal present that indicates the device should enroll.

  // Gather enrollment signals from various sources.
  const base::Value::Dict& device_state =
      local_state->GetDict(prefs::kServerBackedDeviceState);

  const std::string device_state_mode =
      GetString(device_state, kDeviceStateMode);
  const std::string device_state_management_domain =
      GetString(device_state, kDeviceStateManagementDomain);
  const bool is_license_packaged_with_device =
      device_state.FindBool(kDeviceStatePackagedLicense).value_or(false);
  const std::string license_type =
      GetString(device_state, kDeviceStateLicenseType);
  const std::string assigned_upgrade_type = GetString(device_state, kDeviceStateAssignedUpgradeType);

  config.is_license_packaged_with_device = is_license_packaged_with_device;

  if(assigned_upgrade_type == kDeviceStateAssignedUpgradeTypeChromeEnterprise) {
    config.assigned_upgrade_type =
        AssignedUpgradeType::kAssignedUpgradeTypeChromeEnterprise;
  } else if(assigned_upgrade_type == kDeviceStateAssignedUpgradeTypeKiosk) {
    config.assigned_upgrade_type =
        AssignedUpgradeType::kAssignedUpgradeTypeKioskAndSignage;
  }

  config.license_type = GetLicenseTypeToUse(
      license_type, is_license_packaged_with_device, assigned_upgrade_type);

  const bool pref_enrollment_auto_start_present =
      local_state->HasPrefPath(prefs::kDeviceEnrollmentAutoStart);
  const bool pref_enrollment_auto_start =
      local_state->GetBoolean(prefs::kDeviceEnrollmentAutoStart);

  const bool pref_enrollment_can_exit_present =
      local_state->HasPrefPath(prefs::kDeviceEnrollmentCanExit);
  const bool pref_enrollment_can_exit =
      local_state->GetBoolean(prefs::kDeviceEnrollmentCanExit);

  const bool oem_is_managed = ash::system::StatisticsProvider::FlagValueToBool(
      statistics_provider->GetMachineFlag(
          ash::system::kOemIsEnterpriseManagedKey),
      /*default_value=*/false);
  const bool oem_can_exit_enrollment =
      ash::system::StatisticsProvider::FlagValueToBool(
          statistics_provider->GetMachineFlag(
              ash::system::kOemCanExitEnterpriseEnrollmentKey),
          /*default_value=*/true);

  // Decide enrollment mode. Give precedence to forced variants.
  if (IsEnrollingAfterRollback()) {
    config.mode = policy::EnrollmentConfig::MODE_ATTESTATION_ROLLBACK_FORCED;
    config.auth_mechanism =
        policy::EnrollmentConfig::AUTH_MECHANISM_BEST_AVAILABLE;
  } else if (device_state_mode == kDeviceStateRestoreModeReEnrollmentEnforced) {
    config.mode = EnrollmentConfig::MODE_SERVER_FORCED;
    config.management_domain = device_state_management_domain;
  } else if (device_state_mode == kDeviceStateInitialModeEnrollmentEnforced) {
    config.mode = EnrollmentConfig::MODE_INITIAL_SERVER_FORCED;
    config.management_domain = device_state_management_domain;
  } else if (device_state_mode ==
             kDeviceStateRestoreModeReEnrollmentZeroTouch) {
    config.mode = EnrollmentConfig::MODE_ATTESTATION_SERVER_FORCED;
    config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_BEST_AVAILABLE;
    config.management_domain = device_state_management_domain;
  } else if (device_state_mode == kDeviceStateInitialModeEnrollmentZeroTouch) {
    config.mode = EnrollmentConfig::MODE_ATTESTATION_INITIAL_SERVER_FORCED;
    config.auth_mechanism = EnrollmentConfig::AUTH_MECHANISM_BEST_AVAILABLE;
    config.management_domain = device_state_management_domain;
  } else if (pref_enrollment_auto_start_present && pref_enrollment_auto_start &&
             pref_enrollment_can_exit_present && !pref_enrollment_can_exit) {
    config.mode = EnrollmentConfig::MODE_LOCAL_FORCED;
  } else if (oem_is_managed && !oem_can_exit_enrollment) {
    config.mode = EnrollmentConfig::MODE_LOCAL_FORCED;
  } else if (oobe_complete) {
    // If OOBE is complete, don't return advertised modes as there's currently
    // no way to make sure advertised enrollment only gets shown once.
    config.mode = EnrollmentConfig::MODE_NONE;
  } else if (device_state_mode ==
             kDeviceStateRestoreModeReEnrollmentRequested) {
    config.mode = EnrollmentConfig::MODE_SERVER_ADVERTISED;
    config.management_domain = device_state_management_domain;
  } else if (pref_enrollment_auto_start_present && pref_enrollment_auto_start) {
    config.mode = EnrollmentConfig::MODE_LOCAL_ADVERTISED;
  } else if (oem_is_managed) {
    config.mode = EnrollmentConfig::MODE_LOCAL_ADVERTISED;
  }

  return config;
}

// static
EnrollmentConfig::Mode EnrollmentConfig::GetManualFallbackMode(
    EnrollmentConfig::Mode attestation_mode) {
  switch (attestation_mode) {
    case EnrollmentConfig::MODE_ATTESTATION_INITIAL_SERVER_FORCED:
      return EnrollmentConfig::MODE_ATTESTATION_INITIAL_MANUAL_FALLBACK;
    case EnrollmentConfig::MODE_ATTESTATION_SERVER_FORCED:
      return EnrollmentConfig::MODE_ATTESTATION_MANUAL_FALLBACK;
    case EnrollmentConfig::MODE_ATTESTATION_ROLLBACK_FORCED:
      return EnrollmentConfig::MODE_ATTESTATION_ROLLBACK_MANUAL_FALLBACK;
    case EnrollmentConfig::MODE_NONE:
    case EnrollmentConfig::MODE_MANUAL:
    case EnrollmentConfig::MODE_MANUAL_REENROLLMENT:
    case EnrollmentConfig::MODE_LOCAL_FORCED:
    case EnrollmentConfig::MODE_LOCAL_ADVERTISED:
    case EnrollmentConfig::MODE_SERVER_FORCED:
    case EnrollmentConfig::MODE_SERVER_ADVERTISED:
    case EnrollmentConfig::MODE_RECOVERY:
    case EnrollmentConfig::MODE_ATTESTATION:
    case EnrollmentConfig::MODE_ATTESTATION_LOCAL_FORCED:
    case EnrollmentConfig::MODE_ATTESTATION_MANUAL_FALLBACK:
    case EnrollmentConfig::DEPRECATED_MODE_OFFLINE_DEMO:
    case EnrollmentConfig::DEPRECATED_MODE_ENROLLED_ROLLBACK:
    case EnrollmentConfig::MODE_INITIAL_SERVER_FORCED:
    case EnrollmentConfig::MODE_ATTESTATION_INITIAL_MANUAL_FALLBACK:
    case EnrollmentConfig::MODE_ATTESTATION_ROLLBACK_MANUAL_FALLBACK:
      NOTREACHED();
  }
  return EnrollmentConfig::MODE_NONE;
}

std::ostream& operator<<(std::ostream& os, const EnrollmentConfig::Mode& mode) {
  return os << ToStringView(mode);
}

std::ostream& operator<<(std::ostream& os,
                         const EnrollmentConfig::AuthMechanism& auth) {
  return os << ToStringView(auth);
}

std::ostream& operator<<(std::ostream& os, const EnrollmentConfig& config) {
  return os << "EnrollmentConfig(" << config.mode << ", "
            << config.auth_mechanism << ")";
}

}  // namespace policy
