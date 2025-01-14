// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#import "base/strings/sys_string_conversions.h"
#import "components/autofill/core/common/autofill_payments_features.h"
#import "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/autofill/autofill_app_interface.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/earl_grey/earl_grey_test.h"
#import "ios/testing/earl_grey/matchers.h"
#import "net/test/embedded_test_server/default_handlers.h"
#import "ui/base/l10n/l10n_util.h"
#import "ui/base/l10n/l10n_util_mac.h"

namespace {

// The test page url.
const char kCreditCardUrl[] = "/credit_card.html";

// A string on the credit_card.html page used to know when the page has loaded.
const char kAutofillTestString[] = "Autofill Test";

// The name of the card name form input.
const char kFormCardName[] = "CCName";

}  // namespace

// The url to intercept in order to inject card unmask responses. These tests
// do not make requests to the real server.
NSString* const kUnmaskCardRequestUrl =
    @"https://payments.google.com/payments/apis-secure/creditcardservice/"
    @"getrealpan?s7e_suffix=chromewallet";

// The fake response from the payment server when OTP, email and CVC card unmask
// options are available.
NSString* const kUnmaskCardResponseSuccessOtpAndEmailAndCvc =
    @"{\"context_token\":\"__fake_context_token__\",\"idv_challenge_options\":["
    @"{\"sms_otp_challenge_option\":{\"challenge_id\":"
    @"\"JGQ1YTkxM2ZjLWY4YTAtMTFlZS1hMmFhLWZmYjYwNWVjODcwMwo=\",\"masked_phone_"
    @"number\":\"*******1234\",\"otp_length\":6}},{\"email_otp_challenge_"
    @"option\":{\"challenge_id\":"
    @"\"JDNhNTdlMzVhLWY4YTEtMTFlZS1hOTUwLWZiNzY3ZWM4ZWY3ZAo=\",\"masked_email_"
    @"address\":\"a***b@gmail.com\",\"otp_length\":6}},{\"cvc_challenge_"
    @"option\":{\"challenge_id\":\"hardcoded_3CSC_challenge_id\",\"cvc_"
    @"length\":3,\"cvc_position\":\"CVC_POSITION_BACK\"}}]}";

// The url to intercept in order to inject select challenge option responses.
// These tests do not make requests to the real server.
NSString* const kSelectChallengeOptionRequestUrl =
    @"https://payments.google.com/payments/apis/chromepaymentsservice/"
    @"selectchallengeoption";

// The fake response from the payment server when an OTP code is successfully
// sent out.
NSString* const kSelectChallengeOptionResponseSuccess =
    @"{\"context_token\":\"fake_context_token\"}";

// The masked phone number associated with the OTP card unmask option.
NSString* const kUnmaskOptionMaskedPhoneNumber = @"*******1234";

// The masked email associated with the email card unmask option.
NSString* const kUnmaskOptionMaskedEmailAddress = @"a***b@gmail.com";

// The length of the cvc challenge as shown in the challenge message.
NSString* const kUnmaskOptionCvcLength = @"3";

// Matcher for a navigation bar with the "Verification" title.
id<GREYMatcher> CardUnmaskPromptNavigationBarTitle() {
  return chrome_test_util::NavigationBarTitleWithAccessibilityLabelId(
      IDS_AUTOFILL_CARD_UNMASK_PROMPT_NAVIGATION_TITLE_VERIFICATION);
}

// Matcher for the text message challenge option label.
id<GREYMatcher> CardUnmaskTextMessageChallengeOptionLabel() {
  return chrome_test_util::StaticTextWithAccessibilityLabelId(
      IDS_AUTOFILL_AUTHENTICATION_MODE_GET_TEXT_MESSAGE);
}

// Matcher for the CVC challenge option label.
id<GREYMatcher> CardUnmaskCvcChallengeOptionLabel() {
  return chrome_test_util::StaticTextWithAccessibilityLabelId(
      IDS_AUTOFILL_AUTHENTICATION_MODE_SECURITY_CODE);
}

// Matcher for the "Send" button.
id<GREYMatcher> CardUnmaskAuthenticationSelectionSendButton() {
  return grey_allOf(
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_AUTOFILL_CARD_UNMASK_AUTHENTICATION_SELECTION_DIALOG_OK_BUTTON_LABEL_SEND),
      grey_not(grey_accessibilityTrait(UIAccessibilityTraitNotEnabled)), nil);
}

// Matcher for the "Cancel" button.
id<GREYMatcher> CardUnmaskAuthenticationSelectionCancelButton() {
  return grey_allOf(
      chrome_test_util::CancelButton(),
      grey_not(grey_accessibilityTrait(UIAccessibilityTraitNotEnabled)), nil);
}

@interface CardUnmaskAuthenticationSelectionEgtest : ChromeTestCase
@end

@implementation CardUnmaskAuthenticationSelectionEgtest {
  NSString* _enrolledCardNameAndLastFour;
}

#pragma mark - Setup

- (AppLaunchConfiguration)appConfigurationForTestCase {
  AppLaunchConfiguration config;
  config.features_enabled.push_back(
      autofill::features::kAutofillEnableVirtualCards);
  return config;
}

- (void)setUp {
  [super setUp];
  [AutofillAppInterface setUpFakeCreditCardServer];
  _enrolledCardNameAndLastFour =
      [AutofillAppInterface saveMaskedCreditCardEnrolledInVirtualCard];
  [self setUpServer];
  [self setUpTestPage];
}

- (void)setUpServer {
  net::test_server::RegisterDefaultHandlers(self.testServer);
  GREYAssertTrue(self.testServer->Start(), @"Failed to start test server.");
}

- (void)setUpTestPage {
  [ChromeEarlGrey loadURL:self.testServer->GetURL(kCreditCardUrl)];
  [ChromeEarlGrey waitForWebStateContainingText:kAutofillTestString];

  [AutofillAppInterface considerCreditCardFormSecureForTesting];
}

- (void)tearDown {
  [AutofillAppInterface clearAllServerDataForTesting];
  [AutofillAppInterface tearDownFakeCreditCardServer];
  [super tearDown];
}

- (void)showAuthenticationSelection {
  // Tap on the card name field in the web content.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::WebViewMatcher()]
      performAction:chrome_test_util::TapWebElementWithId(kFormCardName)];

  // Wait for the payments bottom sheet to appear.
  id<GREYMatcher> paymentsBottomSheetVirtualCard = grey_accessibilityID(
      [NSString stringWithFormat:@"%@ %@", _enrolledCardNameAndLastFour,
                                 @"Virtual card"]);
  [ChromeEarlGrey
      waitForUIElementToAppearWithMatcher:paymentsBottomSheetVirtualCard];
  [[EarlGrey selectElementWithMatcher:paymentsBottomSheetVirtualCard]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabelId(
                     IDS_IOS_PAYMENT_BOTTOM_SHEET_CONTINUE)]
      performAction:grey_tap()];

  // Wait for the progress dialog to appear.
  [ChromeEarlGrey waitForUIElementToAppearWithMatcher:
                      chrome_test_util::StaticTextWithAccessibilityLabelId(
                          IDS_AUTOFILL_CARD_UNMASK_PROGRESS_DIALOG_TITLE)];

  // Inject the card unmask response with card unmask options.
  [AutofillAppInterface
      setPaymentsResponse:kUnmaskCardResponseSuccessOtpAndEmailAndCvc
               forRequest:kUnmaskCardRequestUrl
            withErrorCode:net::HTTP_OK];

  // Wait for the card unmask authentication selection to appear.
  [ChromeEarlGrey
      waitForUIElementToAppearWithMatcher:CardUnmaskPromptNavigationBarTitle()];
}

- (void)testCardUnmaskAuthenticationSelectionIsShownForVirtualCard {
  [self showAuthenticationSelection];

  // Verify that the card unmask prompt was shown.
  [[EarlGrey
      selectElementWithMatcher:
          chrome_test_util::StaticTextWithAccessibilityLabelId(
              IDS_AUTOFILL_CARD_AUTH_SELECTION_DIALOG_TITLE_MULTIPLE_OPTIONS)]
      assertWithMatcher:grey_sufficientlyVisible()];
  [[EarlGrey
      selectElementWithMatcher:
          chrome_test_util::StaticTextWithAccessibilityLabelId(
              IDS_AUTOFILL_CARD_UNMASK_AUTHENTICATION_SELECTION_DIALOG_ISSUER_CONFIRMATION_TEXT)]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Verify that the card unmask options are shown starting with text message
  // (SMS OTP).
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskTextMessageChallengeOptionLabel()]
      assertWithMatcher:grey_sufficientlyVisible()];
  [[EarlGrey selectElementWithMatcher:chrome_test_util::
                                          StaticTextWithAccessibilityLabel(
                                              kUnmaskOptionMaskedPhoneNumber)]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Verify that the email OTP unmask option is shown.
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabelId(
                     IDS_AUTOFILL_AUTHENTICATION_MODE_GET_EMAIL)]
      assertWithMatcher:grey_sufficientlyVisible()];
  [[EarlGrey selectElementWithMatcher:chrome_test_util::
                                          StaticTextWithAccessibilityLabel(
                                              kUnmaskOptionMaskedEmailAddress)]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Verify that the CVC unmask option is shown.
  [[EarlGrey selectElementWithMatcher:CardUnmaskCvcChallengeOptionLabel()]
      assertWithMatcher:grey_sufficientlyVisible()];
  [[EarlGrey
      selectElementWithMatcher:
          chrome_test_util::StaticTextWithAccessibilityLabel(l10n_util::GetNSStringF(
              IDS_AUTOFILL_CARD_UNMASK_AUTHENTICATION_SELECTION_DIALOG_CVC_CHALLENGE_INFO,
              base::SysNSStringToUTF16(kUnmaskOptionCvcLength),
              l10n_util::GetStringUTF16(
                  IDS_AUTOFILL_CARD_UNMASK_PROMPT_SECURITY_CODE_POSITION_BACK_OF_CARD)))]
      assertWithMatcher:grey_sufficientlyVisible()];
}

- (void)testCardUnmaskAuthenticationSelectionCancel {
  [self showAuthenticationSelection];

  // Tap the cancel button.
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskAuthenticationSelectionCancelButton()]
      performAction:grey_tap()];

  // Expect the card unmask authentication selection view to disappear.
  [ChromeEarlGrey waitForUIElementToDisappearWithMatcher:
                      CardUnmaskPromptNavigationBarTitle()];
}

- (void)testCardUnmaskAuthenticationSelectionAcceptanceButtonLabel {
  [self showAuthenticationSelection];

  // Verify selecting text message sets the acceptance button label to "Send".
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskTextMessageChallengeOptionLabel()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskAuthenticationSelectionSendButton()]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Verify selecting CVC sets the acceptance button label to "Continue".
  [[EarlGrey selectElementWithMatcher:CardUnmaskCvcChallengeOptionLabel()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:
          grey_allOf(
              chrome_test_util::ButtonWithAccessibilityLabelId(
                  IDS_AUTOFILL_CARD_UNMASK_AUTHENTICATION_SELECTION_DIALOG_OK_BUTTON_LABEL_CONTINUE),
              grey_not(grey_accessibilityTrait(UIAccessibilityTraitNotEnabled)),
              nil)] assertWithMatcher:grey_sufficientlyVisible()];
}

- (void)testCardUnmaskAuthenticationSelectionShowsActivityIndicatorView {
  [self showAuthenticationSelection];

  // Select the text message otp challenge option.
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskTextMessageChallengeOptionLabel()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskAuthenticationSelectionSendButton()]
      performAction:grey_tap()];

  // Verify the activity indicator has been set.
  [[EarlGrey
      selectElementWithMatcher:grey_kindOfClassName(@"UIActivityIndicatorView")]
      assertWithMatcher:grey_sufficientlyVisible()];
}

- (void)testDismissInputViaSwipe {
  [self showAuthenticationSelection];

  // The initial access token has been used up, set another fake access token.
  [AutofillAppInterface setAccessToken];
  // Set a fake response for the select challenge option request.
  [AutofillAppInterface
      setPaymentsResponse:kSelectChallengeOptionResponseSuccess
               forRequest:kSelectChallengeOptionRequestUrl
            withErrorCode:net::HTTP_OK];

  // Select the text message otp challenge option.
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskTextMessageChallengeOptionLabel()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:CardUnmaskAuthenticationSelectionSendButton()]
      performAction:grey_tap()];

  // Swipe the input sheet down to close it.
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabelId(
                     IDS_AUTOFILL_CARD_UNMASK_OTP_INPUT_DIALOG_TITLE)]
      performAction:grey_swipeFastInDirection(kGREYDirectionDown)];
}

@end
