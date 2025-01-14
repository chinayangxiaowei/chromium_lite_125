// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/strings/escape.h"
#import "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#import "components/plus_addresses/plus_address_metrics.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/metrics/model/metrics_app_interface.h"
#import "ios/chrome/browser/plus_addresses/ui/plus_address_bottom_sheet_constants.h"
#import "ios/chrome/browser/signin/model/fake_system_identity.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey.h"
#import "ios/chrome/browser/ui/authentication/signin_earl_grey_ui_test_util.h"
#import "ios/chrome/browser/ui/settings/settings_table_view_controller_constants.h"
#import "ios/chrome/common/string_util.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_actions_app_interface.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/web_http_server_chrome_test_case.h"
#import "ios/testing/earl_grey/app_launch_manager.h"
#import "ios/testing/earl_grey/earl_grey_test.h"
#import "ios/testing/earl_grey/matchers.h"
#import "net/test/embedded_test_server/default_handlers.h"
#import "ui/base/l10n/l10n_util_mac.h"

namespace {

constexpr char kEmailFormUrl[] = "/email_signup_form.html";
constexpr char kEmailFieldId[] = "email";
constexpr char kFakeSuggestionLabel[] = "Lorem Ipsum";

// Assert that a given plus address modal event of type `event_type` occurred
// `count` times.
void ExpectModalHistogram(
    plus_addresses::PlusAddressMetrics::PlusAddressModalEvent event_type,
    int count) {
  NSError* error =
      [MetricsAppInterface expectCount:count
                             forBucket:static_cast<int>(event_type)
                          forHistogram:@"Autofill.PlusAddresses.Modal.Events"];
  GREYAssertNil(error, @"Failed to record modal event histogram");
}

// Assert that the bottom sheet shown duration metrics is recorded.
// Actual duration is not assessed to avoid unnecessary clock mocking.
void ExpectModalTimeSample(
    plus_addresses::PlusAddressMetrics::PlusAddressModalCompletionStatus status,
    int count) {
  NSString* modalStatus = [NSString
      stringWithUTF8String:plus_addresses::PlusAddressMetrics::
                               PlusAddressModalCompletionStatusToString(status)
                                   .c_str()];
  NSString* name = [NSString
      stringWithFormat:@"Autofill.PlusAddresses.Modal.%@.ShownDuration",
                       modalStatus];

  NSError* error = [MetricsAppInterface expectTotalCount:count
                                            forHistogram:name];
  GREYAssertNil(error, @"Failed to record modal shown duration histogram");
}

}  // namespace

// Test suite that tests plus addresses functionality.
@interface PlusAddressesTestCase : WebHttpServerChromeTestCase
@end

@implementation PlusAddressesTestCase {
  FakeSystemIdentity* _fakeIdentity;
}

- (void)setUp {
  [super setUp];
  net::test_server::RegisterDefaultHandlers(self.testServer);
  GREYAssertTrue(self.testServer->Start(), @"Server did not start.");
  GREYAssertNil([MetricsAppInterface setupHistogramTester],
                @"Failed to set up histogram tester.");
  // Ensure a fake identity is available, as this is required by the
  // plus_addresses feature.
  _fakeIdentity = [FakeSystemIdentity fakeIdentity1];
  [SigninEarlGrey signinWithFakeIdentity:_fakeIdentity];

  [self loadPlusAddressEligiblePage];
}

- (void)tearDown {
  [super tearDown];
  GREYAssertNil([MetricsAppInterface releaseHistogramTester],
                @"Cannot reset histogram tester.");
}

- (AppLaunchConfiguration)appConfigurationForTestCase {
  AppLaunchConfiguration config;
  // Ensure the feature is enabled, including a required param.
  // TODO(crbug.com/1467623): Set up fake responses via `self.testServer`, or
  // use an app interface to force different states without a backend
  // dependency. The `chrome://version` part in the `server-url` param is just
  // to force an invalid response, and must not be used long-term.
  std::string fakeLocalUrl =
      base::EscapeQueryParamValue("chrome://version", /*use_plus=*/false);
  config.additional_args.push_back(base::StringPrintf(
      "--enable-features=PlusAddressesEnabled:server-url/%s/manage-url/%s",
      fakeLocalUrl.c_str(), fakeLocalUrl.c_str()));
  return config;
}

#pragma mark - Helper methods

// Loads simple page on localhost, ensuring that it is eligible for the
// plus_addresses feature.
- (void)loadPlusAddressEligiblePage {
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kEmailFormUrl)];
  [ChromeEarlGrey waitForWebStateContainingText:"Signup form"];
}

id<GREYMatcher> GetMatcherForSettingsLink() {
  return grey_allOf(
      // The link is within kPlusAddressModalDescriptionAccessibilityIdentifier.
      grey_ancestor(grey_accessibilityID(
          kPlusAddressModalDescriptionAccessibilityIdentifier)),
      // UIKit instantiates a `UIAccessibilityLinkSubelement` for the link
      // element in the label with attributed string.
      grey_kindOfClassName(@"UIAccessibilityLinkSubelement"),
      grey_accessibilityTrait(UIAccessibilityTraitLink), nil);
}

id<GREYMatcher> GetMatcherForErrorReportLink() {
  return grey_allOf(
      // The link is within
      // kPlusAddressModalErrorMessageAccessibilityIdentifier.
      grey_ancestor(grey_accessibilityID(
          kPlusAddressModalErrorMessageAccessibilityIdentifier)),
      // UIKit instantiates a `UIAccessibilityLinkSubelement` for the link
      // element in the label with attributed string.
      grey_kindOfClassName(@"UIAccessibilityLinkSubelement"),
      grey_accessibilityTrait(UIAccessibilityTraitLink), nil);
}

#pragma mark - Tests

// A basic test that simply opens and dismisses the bottom sheet.
- (void)testShowPlusAddressBottomSheet {
  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];
  id<GREYMatcher> user_chip =
      grey_text(base::SysUTF8ToNSString(kFakeSuggestionLabel));

  // Ensure the plus_address suggestion appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:user_chip];

  // Tapping it will trigger the UI.
  // TODO(crbug.com/1467623): Flesh this out as more functionality is
  // implemented. An app interface or demo feature param will be necessary here,
  // too, such that actions that normally trigger server calls can be mocked
  // out.
  [[EarlGrey selectElementWithMatcher:user_chip] performAction:grey_tap()];

  // The primary email address should be shown.
  id<GREYMatcher> primary_email_label = grey_text(_fakeIdentity.userEmail);
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:primary_email_label];

  // The request to reserve a plus address is hitting the test server, and
  // should fail immediately.
  NSString* error_message = l10n_util::GetNSString(
      IDS_PLUS_ADDRESS_MODAL_REPORT_ERROR_INSTRUCTION_IOS);
  id<GREYMatcher> parsed_error_message =
      grey_text(ParseStringWithLinks(error_message).string);
  // Ensure error message with link is shown and correctly parsed.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:parsed_error_message];

  // Ensure the cancel button is shown.
  id<GREYMatcher> cancelButton =
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_PLUS_ADDRESS_MODAL_CANCEL_TEXT);

  // Click the cancel button, dismissing the bottom sheet.
  [[EarlGrey selectElementWithMatcher:cancelButton] performAction:grey_tap()];

  ExpectModalHistogram(
      plus_addresses::PlusAddressMetrics::PlusAddressModalEvent::kModalShown,
      1);
  ExpectModalHistogram(
      plus_addresses::PlusAddressMetrics::PlusAddressModalEvent::kModalCanceled,
      1);
  // The test server currently only response with reserve error. Thus, closing
  // status is recorded as `kReservePlusAddressError`.
  // TODO(b/321072266) Expand coverage to other responses.
  ExpectModalTimeSample(
      plus_addresses::PlusAddressMetrics::PlusAddressModalCompletionStatus::
          kReservePlusAddressError,
      1);
}

- (void)testPlusAddressBottomSheetSettingsLink {
  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];
  id<GREYMatcher> user_chip =
      grey_text(base::SysUTF8ToNSString(kFakeSuggestionLabel));

  // Ensure the plus_address suggestion appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:user_chip];

  [[EarlGrey selectElementWithMatcher:user_chip] performAction:grey_tap()];

  // The settings link should be shown.
  // TODO(crbug.com/1467623): As the link appears inline, the selector seems a
  // little challenging. Hiding it in a private helper for now.
  id<GREYMatcher> link_text = GetMatcherForSettingsLink();

  // Take note of how many tabs are open before clicking the link.
  NSUInteger oldRegularTabCount = [ChromeEarlGrey mainTabCount];
  NSUInteger oldIncognitoTabCount = [ChromeEarlGrey incognitoTabCount];

  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:link_text];
  [[EarlGrey selectElementWithMatcher:link_text] performAction:grey_tap()];

  // A new tab should open after tapping the link.
  [ChromeEarlGrey waitForMainTabCount:oldRegularTabCount + 1];
  [ChromeEarlGrey waitForIncognitoTabCount:oldIncognitoTabCount];

  // The bottom sheet should be dismissed.
  [[EarlGrey selectElementWithMatcher:link_text]
      assertWithMatcher:grey_notVisible()];
}

- (void)testPlusAddressBottomSheetErrorReportLink {
  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];
  id<GREYMatcher> user_chip =
      grey_text(base::SysUTF8ToNSString(kFakeSuggestionLabel));

  // Ensure the plus_address suggestion appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:user_chip];

  [[EarlGrey selectElementWithMatcher:user_chip] performAction:grey_tap()];

  id<GREYMatcher> link_text = GetMatcherForErrorReportLink();

  // Take note of how many tabs are open before clicking the link.
  NSUInteger oldRegularTabCount = [ChromeEarlGrey mainTabCount];
  NSUInteger oldIncognitoTabCount = [ChromeEarlGrey incognitoTabCount];

  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:link_text];
  [[EarlGrey selectElementWithMatcher:link_text] performAction:grey_tap()];

  // A new tab should open after tapping the link.
  [ChromeEarlGrey waitForMainTabCount:oldRegularTabCount + 1];
  [ChromeEarlGrey waitForIncognitoTabCount:oldIncognitoTabCount];

  // The bottom sheet should be dismissed.
  [[EarlGrey selectElementWithMatcher:link_text]
      assertWithMatcher:grey_notVisible()];
}

- (void)testSwipeToDismiss {
  // TODO(crbug.com/1508365): Test fails on iPad.
  if ([ChromeEarlGrey isIPadIdiom]) {
    EARL_GREY_TEST_DISABLED(@"Fails on iPad.");
  }

  // Tap an element that is eligible for plus_address autofilling.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kEmailFieldId)];
  id<GREYMatcher> user_chip =
      grey_text(base::SysUTF8ToNSString(kFakeSuggestionLabel));

  // Ensure the plus_address suggestion appears.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:user_chip];

  // Tapping it will trigger the UI.
  // TODO(crbug.com/1467623): Flesh this out as more functionality is
  // implemented. An app interface or demo feature param will be necessary here,
  // too, such that actions that normally trigger server calls can be mocked
  // out.
  [[EarlGrey selectElementWithMatcher:user_chip] performAction:grey_tap()];

  // The primary email address should be shown.
  id<GREYMatcher> primary_email_label = grey_text(_fakeIdentity.userEmail);
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:primary_email_label];

  // Then, swipe down on the bottom sheet.
  [[EarlGrey selectElementWithMatcher:primary_email_label]
      performAction:grey_swipeSlowInDirection(kGREYDirectionDown)];
  // It should no longer be shown.
  [[EarlGrey selectElementWithMatcher:primary_email_label]
      assertWithMatcher:grey_notVisible()];

  ExpectModalHistogram(
      plus_addresses::PlusAddressMetrics::PlusAddressModalEvent::kModalShown,
      1);
  // TODO(crbug.com/1467623): separate out the cancel click from other exit
  // patterns, on all platforms.
  ExpectModalHistogram(
      plus_addresses::PlusAddressMetrics::PlusAddressModalEvent::kModalCanceled,
      1);
  ExpectModalTimeSample(
      plus_addresses::PlusAddressMetrics::PlusAddressModalCompletionStatus::
          kReservePlusAddressError,
      1);
}

// A test to ensure that a row in the settings view shows up for
// plus_addresses, and that tapping it opens a new tab for its settings, which
// are managed externally.
- (void)testSettings {
  [ChromeEarlGreyUI openSettingsMenu];
  // Take note of how many tabs are open before clicking the link in settings,
  // which should simply open a new tab.
  NSUInteger oldRegularTabCount = [ChromeEarlGrey mainTabCount];
  NSUInteger oldIncognitoTabCount = [ChromeEarlGrey incognitoTabCount];
  [ChromeEarlGreyUI
      tapSettingsMenuButton:grey_accessibilityID(kSettingsPlusAddressesId)];

  // A new tab should open after tapping the link.
  [ChromeEarlGrey waitForMainTabCount:oldRegularTabCount + 1];
  [ChromeEarlGrey waitForIncognitoTabCount:oldIncognitoTabCount];
}

@end
