// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/google_services/manage_accounts/accounts_table_view_controller.h"

#import "base/apple/foundation_util.h"
#import "base/functional/callback_helpers.h"
#import "base/run_loop.h"
#import "base/strings/sys_string_conversions.h"
#import "components/sync/service/sync_service.h"
#import "components/sync/test/test_sync_service.h"
#import "components/variations/scoped_variations_ids_provider.h"
#import "google_apis/gaia/core_account_id.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser/test/test_browser.h"
#import "ios/chrome/browser/shared/model/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/shared/public/commands/application_commands.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/table_view/legacy_chrome_table_view_controller_test.h"
#import "ios/chrome/browser/signin/model/authentication_service.h"
#import "ios/chrome/browser/signin/model/authentication_service_factory.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service_factory.h"
#import "ios/chrome/browser/signin/model/fake_authentication_service_delegate.h"
#import "ios/chrome/browser/signin/model/fake_system_identity.h"
#import "ios/chrome/browser/signin/model/fake_system_identity_manager.h"
#import "ios/chrome/browser/signin/model/identity_manager_factory.h"
#import "ios/chrome/browser/sync/model/sync_service_factory.h"
#import "ios/chrome/browser/ui/settings/google_services/manage_accounts/accounts_mediator.h"
#import "ios/chrome/test/ios_chrome_scoped_testing_local_state.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/public/test/web_task_environment.h"
#import "testing/gmock/include/gmock/gmock.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

namespace {

std::unique_ptr<KeyedService> CreateTestSyncService(
    web::BrowserState* context) {
  return std::make_unique<syncer::TestSyncService>();
}

void SetSyncStateFeatureActive(const CoreAccountInfo& account,
                               syncer::TestSyncService* sync_service) {
  sync_service->SetAccountInfo(account);
  sync_service->SetHasSyncConsent(true);
  sync_service->SetTransportState(syncer::SyncService::TransportState::ACTIVE);
  sync_service->SetDisableReasons({});
  sync_service->SetInitialSyncFeatureSetupComplete(true);
  ASSERT_TRUE(sync_service->IsSyncFeatureEnabled());
}

void SetSyncStateTransportActive(const CoreAccountInfo& account,
                                 syncer::TestSyncService* sync_service) {
  sync_service->SetAccountInfo(account);
  sync_service->SetHasSyncConsent(false);
  sync_service->SetTransportState(syncer::SyncService::TransportState::ACTIVE);
  sync_service->SetDisableReasons({});
  ASSERT_FALSE(sync_service->IsSyncFeatureEnabled());
}

}  // namespace

class AccountsTableViewControllerTest
    : public LegacyChromeTableViewControllerTest {
 public:
  AccountsTableViewControllerTest()
      : task_environment_(web::WebTaskEnvironment::IO_MAINLOOP) {
    TestChromeBrowserState::Builder builder;
    builder.AddTestingFactory(
        AuthenticationServiceFactory::GetInstance(),
        AuthenticationServiceFactory::GetDefaultFactory());
    builder.AddTestingFactory(SyncServiceFactory::GetInstance(),
                              base::BindRepeating(&CreateTestSyncService));
    browser_state_ = builder.Build();
    browser_ = std::make_unique<TestBrowser>(browser_state_.get());

    AuthenticationServiceFactory::CreateAndInitializeForBrowserState(
        browser_state_.get(),
        std::make_unique<FakeAuthenticationServiceDelegate>());
  }

  LegacyChromeTableViewController* InstantiateController() override {
    // Set up ApplicationCommands mock.
    id mock_application_handler =
        OCMProtocolMock(@protocol(ApplicationCommands));
    CommandDispatcher* dispatcher = browser_->GetCommandDispatcher();
    [dispatcher startDispatchingToTarget:mock_application_handler
                             forProtocol:@protocol(ApplicationCommands)];

    AccountsMediator* mediator =
        [[AccountsMediator alloc] initWithSyncService:test_sync_service()
                                accountManagerService:account_manager_service()
                                          authService:authentication_service()
                                      identityManager:identity_manager()];

    AccountsTableViewController* controller =
        [[AccountsTableViewController alloc]
                                initWithBrowser:browser_.get()
                      closeSettingsOnAddAccount:NO
                     applicationCommandsHandler:mock_application_handler
            signoutDismissalByParentCoordinator:NO];

    mediator.consumer = controller;
    controller.modelIdentityDataSource = mediator;
    mediator_ = mediator;

    return controller;
  }

  void TearDown() override {
    mediator_ = nil;
    [base::apple::ObjCCast<AccountsTableViewController>(controller())
        settingsWillBeDismissed];
    LegacyChromeTableViewControllerTest::TearDown();
  }

  // Identity Services
  signin::IdentityManager* identity_manager() {
    return IdentityManagerFactory::GetForBrowserState(browser_state_.get());
  }

  AuthenticationService* authentication_service() {
    return AuthenticationServiceFactory::GetForBrowserState(
        browser_state_.get());
  }

  FakeSystemIdentityManager* fake_system_identity_manager() {
    return FakeSystemIdentityManager::FromSystemIdentityManager(
        GetApplicationContext()->GetSystemIdentityManager());
  }

  syncer::TestSyncService* test_sync_service() {
    return static_cast<syncer::TestSyncService*>(
        SyncServiceFactory::GetForBrowserState(browser_state_.get()));
  }

  ChromeAccountManagerService* account_manager_service() {
    return ChromeAccountManagerServiceFactory::GetForBrowserState(
        browser_state_.get());
  }

 private:
  web::WebTaskEnvironment task_environment_;
  IOSChromeScopedTestingLocalState local_state_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  std::unique_ptr<Browser> browser_;
  variations::ScopedVariationsIdsProvider scoped_variations_ids_provider_{
      variations::VariationsIdsProvider::Mode::kUseSignedInState};
  AccountsMediator* mediator_;
};

// Tests that a valid identity is added to the model.
TEST_F(AccountsTableViewControllerTest, AddChromeIdentity) {
  FakeSystemIdentity* identity =
      [FakeSystemIdentity identityWithEmail:@"foo1@gmail.com"
                                     gaiaID:@"foo1ID"
                                       name:@"Fake Foo 1"];
  fake_system_identity_manager()->AddIdentity(identity);

  // Simulates a credential reload.
  authentication_service()->SignIn(
      identity, signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
  fake_system_identity_manager()->FireSystemIdentityReloaded();
  base::RunLoop().RunUntilIdle();

  CreateController();
  CheckController();

  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(2, NumberOfItemsInSection(0));
}

// Tests that an invalid identity is not added to the model.
TEST_F(AccountsTableViewControllerTest, IgnoreMismatchWithAccountInfo) {
  FakeSystemIdentity* identity1 =
      [FakeSystemIdentity identityWithEmail:@"foo1@gmail.com"
                                     gaiaID:@"foo1ID"
                                       name:@"Fake Foo 1"];
  FakeSystemIdentity* identity2 =
      [FakeSystemIdentity identityWithEmail:@"foo2@gmail.com"
                                     gaiaID:@"foo2ID"
                                       name:@"Fake Foo 2"];
  fake_system_identity_manager()->AddIdentity(identity1);
  fake_system_identity_manager()->AddIdentity(identity2);

  // Simulates a credential reload.
  authentication_service()->SignIn(
      identity1, signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
  fake_system_identity_manager()->FireSystemIdentityReloaded();
  base::RunLoop().RunUntilIdle();

  CreateController();
  CheckController();

  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(3, NumberOfItemsInSection(0));

  // Removes identity2 from identity service but not account info storage. This
  // is an asynchronous call, so wait for completion.
  {
    base::RunLoop run_loop;
    fake_system_identity_manager()->ForgetIdentity(
        identity2, base::IgnoreArgs<NSError*>(run_loop.QuitClosure()));
    run_loop.Run();
  }

  [controller() loadModel];

  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(2, NumberOfItemsInSection(0));
}

// Tests that no passphrase error is exposed in the account table view (since
// it's exposed one level up).
TEST_F(AccountsTableViewControllerTest, DontHoldPassphraseError) {
  const std::string email = "foo@gmail.com";
  const std::string gaia_id = "fooID";

  FakeSystemIdentity* identity =
      [FakeSystemIdentity identityWithEmail:base::SysUTF8ToNSString(email)
                                     gaiaID:base::SysUTF8ToNSString(gaia_id)
                                       name:@"Fake Foo"];
  fake_system_identity_manager()->AddIdentity(identity);

  // Simulate a credential reload.
  authentication_service()->SignIn(
      identity, signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
  fake_system_identity_manager()->FireSystemIdentityReloaded();
  base::RunLoop().RunUntilIdle();

  CoreAccountInfo account;
  account.email = email;
  account.gaia = gaia_id;
  account.account_id = CoreAccountId::FromGaiaId(account.gaia);
  SetSyncStateTransportActive(account, test_sync_service());
  test_sync_service()->SetPassphraseRequiredForPreferredDataTypes(true);

  CreateController();
  CheckController();

  EXPECT_EQ(2, NumberOfSections());
}

// Tests that when eligible the account model doesn't have the Account Storage
// error when there is no error.
TEST_F(AccountsTableViewControllerTest,
       DontHoldPassphraseErrorWhenEligibleNoError) {
  const std::string email = "foo@gmail.com";
  const std::string gaia_id = "fooID";

  FakeSystemIdentity* identity =
      [FakeSystemIdentity identityWithEmail:base::SysUTF8ToNSString(email)
                                     gaiaID:base::SysUTF8ToNSString(gaia_id)
                                       name:@"Fake Foo"];
  fake_system_identity_manager()->AddIdentity(identity);

  // Simulate a credential reload.
  authentication_service()->SignIn(
      identity, signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
  fake_system_identity_manager()->FireSystemIdentityReloaded();
  base::RunLoop().RunUntilIdle();

  CoreAccountInfo account;
  account.email = email;
  account.gaia = gaia_id;
  account.account_id = CoreAccountId::FromGaiaId(account.gaia);
  SetSyncStateFeatureActive(account, test_sync_service());
  test_sync_service()->SetPassphraseRequiredForPreferredDataTypes(false);

  CreateController();
  CheckController();

  // Verify that there are only 2 sections, exluding the error section.
  EXPECT_EQ(2, NumberOfSections());
}
