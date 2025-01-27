// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <memory>

#import "base/strings/sys_string_conversions.h"
#import "base/strings/utf_string_conversions.h"
#import "base/test/ios/wait_util.h"
#import "base/time/time.h"
#import "components/autofill/core/common/autofill_features.h"
#import "components/autofill/ios/common/features.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/signin/model/fake_system_identity.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey_ui_test_util.h"
#import "ios/chrome/browser/ui/autofill/autofill_app_interface.h"
#import "ios/chrome/browser/ui/infobars/banners/infobar_banner_constants.h"
#import "ios/chrome/browser/ui/infobars/infobar_earl_grey_ui_test_util.h"
#import "ios/chrome/browser/ui/infobars/modals/infobar_address_profile_modal_constants.h"
#import "ios/chrome/grit/ios_branded_strings.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/earl_grey/earl_grey_test.h"
#import "ios/web/public/test/http_server/http_server.h"
#import "net/test/embedded_test_server/embedded_test_server.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "ui/base/l10n/l10n_util.h"

using base::test::ios::kWaitForActionTimeout;

namespace {

// URLs of the test pages.
constexpr char kProfileForm[] = "/autofill_smoke_test.html";

// Ids of fields in the form.
constexpr char kFormElementName[] = "form_name";
constexpr char kFormElementEmail[] = "form_email";
constexpr char kFormElementSubmit[] = "submit_profile";

// Email value used by the tests.
constexpr char kEmail[] = "foo1@gmail.com";

// Matcher for the banner button.
id<GREYMatcher> BannerButtonMatcher() {
  return grey_accessibilityLabel(l10n_util::GetNSString(
      IDS_IOS_AUTOFILL_SAVE_ADDRESS_MESSAGE_PRIMARY_ACTION));
}

// Matcher for the update banner button.
id<GREYMatcher> UpdateBannerButtonMatcher() {
  return grey_accessibilityLabel(l10n_util::GetNSString(
      IDS_IOS_AUTOFILL_UPDATE_ADDRESS_MESSAGE_PRIMARY_ACTION));
}

// Matcher for the "Save Address" modal button.
id<GREYMatcher> ModalButtonMatcher() {
  return grey_allOf(grey_accessibilityLabel(l10n_util::GetNSString(
                        IDS_AUTOFILL_SAVE_ADDRESS_PROMPT_OK_BUTTON_LABEL)),
                    grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
}

// Matcher for the "Update Address" modal button.
id<GREYMatcher> UpdateModalButtonMatcher() {
  return grey_allOf(grey_accessibilityLabel(l10n_util::GetNSString(
                        IDS_AUTOFILL_UPDATE_ADDRESS_PROMPT_OK_BUTTON_LABEL)),
                    grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
}

// Matcher for the modal button.
id<GREYMatcher> ModalEditButtonMatcher() {
  return grey_allOf(grey_accessibilityID(kInfobarSaveAddressModalEditButton),
                    grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
}

// Matcher for the migration button in modal view.
id<GREYMatcher> ModalMigrationButtonMatcher() {
  return grey_allOf(
      grey_accessibilityLabel(l10n_util::GetNSString(
          IDS_AUTOFILL_ADDRESS_MIGRATION_TO_ACCOUNT_PROMPT_OK_BUTTON_LABEL)),
      grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
}

// Waits for the keyboard to appear. Returns NO on timeout.
BOOL WaitForKeyboardToAppear() {
  GREYCondition* waitForKeyboard = [GREYCondition
      conditionWithName:@"Wait for keyboard"
                  block:^BOOL {
                    return [EarlGrey isKeyboardShownWithError:nil];
                  }];
  return [waitForKeyboard waitWithTimeout:kWaitForActionTimeout.InSecondsF()];
}

}  // namespace

@interface SaveProfileEGTest : ChromeTestCase

@end

@implementation SaveProfileEGTest

- (void)tearDown {
  // Clear existing profile.
  [AutofillAppInterface clearProfilesStore];

  [super tearDown];
}

- (AppLaunchConfiguration)appConfigurationForTestCase {
  AppLaunchConfiguration config;

  if ([self isRunningTest:@selector
            (DISABLED_testUserData_LocalEditViaBottomSheet)]) {
    config.features_enabled.push_back(
        kAutofillDynamicallyLoadsFieldsForAddressInput);
  }

  config.features_disabled.push_back(
      autofill::features::test::kAutofillServerCommunication);

  if ([self isRunningTest:@selector(testStickySavePromptJourney)]) {
    config.features_enabled.push_back(kAutofillStickyInfobarIos);
  }

  return config;
}

#pragma mark - Test helper methods

// Fills the president profile in the form by clicking on the button, submits
// the form to the save address profile infobar.
- (void)fillPresidentProfileAndShowSaveInfobar {
  GREYAssertTrue(self.testServer->Start(), @"Server did not start.");
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kProfileForm)];

  // Ensure there are no saved profiles.
  GREYAssertEqual(0U, [AutofillAppInterface profilesCount],
                  @"There should be no saved profile.");

  [ChromeEarlGrey tapWebStateElementWithID:@"fill_profile_president"];
  [ChromeEarlGrey tapWebStateElementWithID:@"submit_profile"];
  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:YES];
}

- (void)fillPresidentProfileAndShowSaveModal {
  [self fillPresidentProfileAndShowSaveInfobar];

  // Accept the banner.
  [[EarlGrey selectElementWithMatcher:BannerButtonMatcher()]
      performAction:grey_tap()];
  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:NO];
}

// Fills, submits the form and saves the address profile.
- (void)fillFormAndSaveProfile {
  [self fillPresidentProfileAndShowSaveModal];

  // Save the profile.
  [[EarlGrey selectElementWithMatcher:ModalButtonMatcher()]
      performAction:grey_tap()];

  // Ensure profile is saved locally.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been saved.");
}

// Focuses on the name field and initiates autofill on the form with the saved
// profile.
- (void)focusOnNameAndAutofill {
  // Ensure there is a saved local profile.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"There should a saved local profile.");

  // Tap on a field to trigger form activity.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kFormElementName)];

  // Wait for the keyboard to appear.
  WaitForKeyboardToAppear();

  // Tap on the suggestion.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::
                                          AutofillSuggestionViewMatcher()]
      performAction:grey_tap()];

  // Verify Web Content was filled.
  NSString* javaScriptCondition = [NSString
      stringWithFormat:@"document.getElementById('%s').value.length > 0",
                       kFormElementName];
  [ChromeEarlGrey waitForJavaScriptCondition:javaScriptCondition];
}

#pragma mark - Tests

// Ensures that the profile is updated after submitting the form.
- (void)testUserData_LocalUpdate {
  // Save a profile.
  [self fillFormAndSaveProfile];

  // Load the form.
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kProfileForm)];

  // Autofill the form.
  [self focusOnNameAndAutofill];

  // Tap on email field.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kFormElementEmail)];

  // Wait for the keyboard to appear.
  WaitForKeyboardToAppear();

  // Populate the email field.
  // TODO(crbug.com/1454516): This should use grey_typeText when fixed.
  for (int i = 0; kEmail[i] != '\0'; ++i) {
    NSString* letter = base::SysUTF8ToNSString(std::string(1, kEmail[i]));
    if (kEmail[i] == '@') {
      [ChromeEarlGrey simulatePhysicalKeyboardEvent:letter
                                              flags:UIKeyModifierShift];
      continue;
    }

    [ChromeEarlGrey simulatePhysicalKeyboardEvent:letter flags:0];
  }

  // Submit the form.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kFormElementSubmit)];

  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:YES];

  // Accept the banner.
  [[EarlGrey selectElementWithMatcher:UpdateBannerButtonMatcher()]
      performAction:grey_tap()];
  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:NO];

  // Update the profile.
  [[EarlGrey selectElementWithMatcher:UpdateModalButtonMatcher()]
      performAction:grey_tap()];

  // Ensure profile is updated locally.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been updated.");
}

// Ensures that the profile is saved to Chrome after submitting and editing the
// form.
- (void)testUserData_LocalEdit {
  // Fill and submit the form.
  [self fillPresidentProfileAndShowSaveModal];

  // Edit the profile.
  [[EarlGrey selectElementWithMatcher:ModalEditButtonMatcher()]
      performAction:grey_tap()];

  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::TextFieldForCellWithLabelId(
                                   IDS_IOS_AUTOFILL_CITY)]
      performAction:grey_replaceText(@"New York")];

  // Save the profile.
  [[EarlGrey selectElementWithMatcher:ModalButtonMatcher()]
      performAction:grey_tap()];

  // Ensure profile is saved locally.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been saved.");
}

// Ensures that the profile is saved to Account after submitting the form.
- (void)testUserData_AccountSave {
  [SigninEarlGrey signinWithFakeIdentity:[FakeSystemIdentity fakeIdentity1]];

  [self fillPresidentProfileAndShowSaveModal];

  id<GREYMatcher> footerMatcher = grey_text(
      l10n_util::GetNSStringF(IDS_IOS_AUTOFILL_SAVE_ADDRESS_IN_ACCOUNT_FOOTER,
                              base::UTF8ToUTF16(std::string(kEmail))));

  [[EarlGrey selectElementWithMatcher:footerMatcher]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Save the profile.
  [[EarlGrey selectElementWithMatcher:ModalButtonMatcher()]
      performAction:grey_tap()];

  // Ensure profile is saved locally.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been saved.");

  [SigninEarlGrey signOut];
}

// Ensures that the profile is saved to Account after submitting and editing the
// form.
- (void)testUserData_AccountEdit {
  [SigninEarlGrey signinWithFakeIdentity:[FakeSystemIdentity fakeIdentity1]];

  [self fillPresidentProfileAndShowSaveModal];

  // Edit the profile.
  [[EarlGrey selectElementWithMatcher:ModalEditButtonMatcher()]
      performAction:grey_tap()];

  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::TextFieldForCellWithLabelId(
                                   IDS_IOS_AUTOFILL_CITY)]
      performAction:grey_replaceText(@"New York")];

  id<GREYMatcher> footerMatcher = grey_text(
      l10n_util::GetNSStringF(IDS_IOS_AUTOFILL_SAVE_ADDRESS_IN_ACCOUNT_FOOTER,
                              base::UTF8ToUTF16(std::string(kEmail))));

  [[EarlGrey selectElementWithMatcher:footerMatcher]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Save the profile.
  [[EarlGrey selectElementWithMatcher:ModalButtonMatcher()]
      performAction:grey_tap()];

  // Ensure profile is saved locally.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been saved.");

  [SigninEarlGrey signOut];
}

// Ensures that if a local profile is filled in a form and submitted, the user
// is asked for a migration prompt and the profile is moved to the Account.
- (void)testUserData_MigrationToAccount {
  [AutofillAppInterface clearProfilesStore];

  // Store one local address.
  [AutofillAppInterface saveExampleProfile];

  [SigninEarlGrey signinWithFakeIdentity:[FakeSystemIdentity fakeIdentity1]];

  GREYAssertTrue(self.testServer->Start(), @"Server did not start.");
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kProfileForm)];

  [self focusOnNameAndAutofill];

  // Submit the form.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kFormElementSubmit)];

  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:YES];

  // Accept the banner.
  [[EarlGrey selectElementWithMatcher:BannerButtonMatcher()]
      performAction:grey_tap()];
  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:NO];

  id<GREYMatcher> footerMatcher = grey_text(l10n_util::GetNSStringF(
      IDS_IOS_AUTOFILL_ADDRESS_MIGRATE_IN_ACCOUNT_FOOTER,
      base::UTF8ToUTF16(std::string(kEmail))));
  // Check if there is footer suggesting it's a migration prompt.
  [[EarlGrey selectElementWithMatcher:footerMatcher]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Confirm to migrate the profile.
  [[EarlGrey selectElementWithMatcher:ModalMigrationButtonMatcher()]
      performAction:grey_tap()];

  // Ensure the count of profiles saved remains unchanged.
  GREYAssertEqual(1U, [AutofillAppInterface profilesCount],
                  @"Profile should have been saved.");

  [SigninEarlGrey signOut];
}

// Tests that the user can edit a field in the edit via in the save address
// flow.
// TODO(crbug.com/1482269): Re-enable the test when the save functionality is
// implemented. Currently, the test appears flaky beacuse after a certain number
// of tries, the save prompt is not presented if no action is taken on it.
- (void)DISABLED_testUserData_LocalEditViaBottomSheet {
  // Fill and submit the form.
  [self fillPresidentProfileAndShowSaveModal];

  // Edit the profile.
  [[EarlGrey selectElementWithMatcher:ModalEditButtonMatcher()]
      performAction:grey_tap()];

  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::TextFieldForCellWithLabelId(
                                   IDS_IOS_AUTOFILL_CITY)]
      performAction:grey_replaceText(@"New York")];

  // TODO(crbug.com/1482269): Update test when save functionality is
  // implemented.
}

// Tests the sticky address prompt journey where the prompt remains there when
// navigating without an explicit user gesture, and then the prompt is dismissed
// when navigating with a user gesture. Test with the address save prompt but
// the type of address prompt doesn't matter in this test case.
- (void)testStickySavePromptJourney {
  [self fillPresidentProfileAndShowSaveInfobar];

  {
    // Reloading page from script shouldn't dismiss the infobar.
    NSString* script = @"location.reload();";
    [ChromeEarlGrey evaluateJavaScriptForSideEffect:script];
  }
  {
    // Assigning url from script to the page aka open an url shouldn't dismiss
    // the infobar.
    NSString* script = @"window.location.assign(window.location.href);";
    [ChromeEarlGrey evaluateJavaScriptForSideEffect:script];
  }
  {
    // Pushing new history entry without reloading content shouldn't dismiss the
    // infobar.
    NSString* script = @"history.pushState({}, '', 'destination2.html');";
    [ChromeEarlGrey evaluateJavaScriptForSideEffect:script];
  }
  {
    // Replacing history entry without reloading content shouldn't dismiss the
    // infobar.
    NSString* script = @"history.replaceState({}, '', 'destination3.html');";
    [ChromeEarlGrey evaluateJavaScriptForSideEffect:script];
  }

  // Wait some time for things to settle.
  base::test::ios::SpinRunLoopWithMinDelay(base::Milliseconds(200));

  // Verify that the prompt is still there after the non-user initiated
  // navigations.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kInfobarBannerViewIdentifier)]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Navigate with an emulated user gesture and verify that dismisses the
  // prompt.
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kProfileForm)];
  [InfobarEarlGreyUI waitUntilInfobarBannerVisibleOrTimeout:NO];
}

@end
