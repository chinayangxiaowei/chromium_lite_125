// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_trust_checker.h"

#include <memory>

#include "base/check_deref.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "base/types/cxx23_to_underlying.h"
#include "chrome/browser/policy/developer_tools_policy_handler.h"
#include "chrome/browser/web_applications/test/web_app_test.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "components/web_package/mojom/web_bundle_parser.mojom.h"
#include "components/web_package/signed_web_bundles/ed25519_public_key.h"
#include "components/web_package/signed_web_bundles/signed_web_bundle_id.h"
#include "components/web_package/signed_web_bundles/signed_web_bundle_integrity_block.h"
#include "content/public/common/content_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if BUILDFLAG(IS_CHROMEOS)
#include "base/values.h"
#include "chrome/browser/web_applications/isolated_web_apps/policy/isolated_web_app_policy_constants.h"
#endif  // BUILDFLAG(IS_CHROMEOS)

namespace web_app {

namespace {

constexpr std::array<uint8_t, 32> kPublicKeyBytes1 = {
    0x01, 0x23, 0x43, 0x43, 0x33, 0x42, 0x7A, 0x14, 0x42, 0x14, 0xa2,
    0xb6, 0xc2, 0xd9, 0xf2, 0x02, 0x03, 0x42, 0x18, 0x10, 0x12, 0x26,
    0x62, 0x88, 0xf6, 0xa3, 0xa5, 0x47, 0x14, 0x69, 0x00, 0x73};

constexpr std::array<uint8_t, 32> kPublicKeyBytes2 = {
    0x02, 0x23, 0x43, 0x43, 0x33, 0x42, 0x7A, 0x14, 0x42, 0x14, 0xa2,
    0xb6, 0xc2, 0xd9, 0xf2, 0x02, 0x03, 0x42, 0x18, 0x10, 0x12, 0x26,
    0x62, 0x88, 0xf6, 0xa3, 0xa5, 0x47, 0x14, 0x69, 0x00, 0x73};

}  // namespace

class IsolatedWebAppTrustCheckerTest : public WebAppTest {
 public:
  IsolatedWebAppTrustCheckerTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kIsolatedWebApps);
  }

  void SetUp() override {
    WebAppTest::SetUp();

    isolated_web_app_trust_checker_ =
        std::make_unique<IsolatedWebAppTrustChecker>(*profile());
  }

  void TearDown() override {
    isolated_web_app_trust_checker_.reset();

    WebAppTest::TearDown();
  }

  IsolatedWebAppTrustChecker& trust_checker() {
    return *isolated_web_app_trust_checker_;
  }

  PrefService& pref_service() { return *profile()->GetPrefs(); }

  const web_package::Ed25519PublicKey kPublicKey1 =
      web_package::Ed25519PublicKey::Create(base::make_span(kPublicKeyBytes1));
  const web_package::Ed25519PublicKey kPublicKey2 =
      web_package::Ed25519PublicKey::Create(base::make_span(kPublicKeyBytes2));

  const web_package::SignedWebBundleId kWebBundleId1 =
      web_package::SignedWebBundleId::CreateForEd25519PublicKey(kPublicKey1);
  const web_package::SignedWebBundleId kWebBundleId2 =
      web_package::SignedWebBundleId::CreateForEd25519PublicKey(kPublicKey2);

  const GURL kStartUrl1 =
      GURL(std::string(chrome::kIsolatedAppScheme) +
           url::kStandardSchemeSeparator + kWebBundleId1.id());
  const GURL kStartUrl2 =
      GURL(std::string(chrome::kIsolatedAppScheme) +
           url::kStandardSchemeSeparator + kWebBundleId2.id());

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<IsolatedWebAppTrustChecker> isolated_web_app_trust_checker_;
};

TEST_F(IsolatedWebAppTrustCheckerTest, DevWebBundleId) {
  IsolatedWebAppTrustChecker::Result result = trust_checker().IsTrusted(
      web_package::SignedWebBundleId::CreateRandomForProxyMode(),
      /*is_dev_mode_bundle=*/false);
  EXPECT_EQ(result.status, IsolatedWebAppTrustChecker::Result::Status::
                               kErrorUnsupportedWebBundleIdType);
}

TEST_F(IsolatedWebAppTrustCheckerTest, UntrustedByDefault) {
  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }

  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId2,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }
}

#if BUILDFLAG(IS_CHROMEOS)

TEST_F(IsolatedWebAppTrustCheckerTest, TrustedViaPolicy) {
  base::Value::List force_install_list;
  {
    base::Value::Dict force_install_entry;
    force_install_entry.Set(kPolicyWebBundleIdKey, "not a web bundle id");
    force_install_entry.Set(kPolicyUpdateManifestUrlKey,
                            "https://example.com/update-manifest.json");
    force_install_list.Append(std::move(force_install_entry));
  }
  {
    base::Value::Dict force_install_entry;
    force_install_entry.Set(kPolicyWebBundleIdKey, kWebBundleId1.id());
    force_install_entry.Set(kPolicyUpdateManifestUrlKey,
                            "https://example.com/update-manifest.json");
    force_install_list.Append(std::move(force_install_entry));
  }
  pref_service().SetList(prefs::kIsolatedWebAppInstallForceList,
                         std::move(force_install_list));

  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(result.status,
              IsolatedWebAppTrustChecker::Result::Status::kTrusted);
  }

  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId2,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }
}

#endif  // BUILDFLAG(IS_CHROMEOS)

TEST_F(IsolatedWebAppTrustCheckerTest,
       DevModeDoesNotAutomaticallyTrustAllApps) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kIsolatedWebAppDevMode);

  IsolatedWebAppTrustChecker::Result result =
      trust_checker().IsTrusted(kWebBundleId1,
                                /*is_dev_mode_bundle=*/false);
  EXPECT_EQ(
      result.status,
      IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
}

TEST_F(IsolatedWebAppTrustCheckerTest, TrustedViaDevMode) {
  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/true);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kIsolatedWebAppDevMode);
  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/true);
    EXPECT_EQ(result.status,
              IsolatedWebAppTrustChecker::Result::Status::kTrusted);
  }
  pref_service().SetInteger(
      prefs::kDevToolsAvailability,
      base::to_underlying(
          policy::DeveloperToolsPolicyHandler::Availability::kDisallowed));
  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/true);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }
}

TEST_F(IsolatedWebAppTrustCheckerTest, TrustedWebBundleIDsForTesting) {
  SetTrustedWebBundleIdsForTesting({kWebBundleId1});

  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId1,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(result.status,
              IsolatedWebAppTrustChecker::Result::Status::kTrusted);
  }

  {
    IsolatedWebAppTrustChecker::Result result =
        trust_checker().IsTrusted(kWebBundleId2,
                                  /*is_dev_mode_bundle=*/false);
    EXPECT_EQ(
        result.status,
        IsolatedWebAppTrustChecker::Result::Status::kErrorPublicKeysNotTrusted);
  }
}

}  // namespace web_app
