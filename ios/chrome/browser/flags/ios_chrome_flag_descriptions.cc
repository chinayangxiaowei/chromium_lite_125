// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/flags/ios_chrome_flag_descriptions.h"

// This file declares strings used in chrome://flags. These messages are not
// translated, because instead of end-users they target Chromium developers and
// testers. See https://crbug.com/587272 and https://crbug.com/703134 for more
// details.

namespace flag_descriptions {

const char kAutofillCreditCardUploadName[] =
    "Offers uploading Autofilled credit cards";
const char kAutofillCreditCardUploadDescription[] =
    "Offers uploading Autofilled credit cards to Google Payments after form "
    "submission.";

const char kAutofillDisableProfileUpdatesName[] =
    "Disables Autofill profile updates from form submissions";
const char kAutofillDisableProfileUpdatesDescription[] =
    "When enabled, Autofill will not apply updates to address profiles based "
    "on data extracted from submitted forms. For testing purposes.";

const char kAutofillDisableSilentProfileUpdatesName[] =
    "Disables Autofill silent profile updates from form submissions";
const char kAutofillDisableSilentProfileUpdatesDescription[] =
    "When enabled, Autofill will not apply silent updates to address profiles. "
    "For testing purposes.";

const char kAutofillEnableCardArtImageName[] = "Enable showing card art images";
const char kAutofillEnableCardArtImageDescription[] =
    "When enabled, card product images (instead of network icons) will be "
    "shown in Payments Autofill UI.";

const char kAutofillEnableCardBenefitsForAmericanExpressName[] =
    "Enable showing American Express card benefits";
const char kAutofillEnableCardBenefitsForAmericanExpressDescription[] =
    "When enabled, card benefits offered by American Express will be shown in "
    "Autofill suggestions.";

const char kAutofillEnableCardBenefitsForCapitalOneName[] =
    "Enable showing Capital One card benefits";
const char kAutofillEnableCardBenefitsForCapitalOneDescription[] =
    "When enabled, card benefits offered by Capital One will be shown in "
    "Autofill suggestions.";

const char kAutofillEnableCardBenefitsSyncName[] =
    "Enable syncing card benefits from the server";
const char kAutofillEnableCardBenefitsSyncDescription[] =
    "When enabled, card benefits offered by issuers will be synced from "
    "the Payments server.";

const char kAutofillEnableDynamicallyLoadingFieldsForAddressInputName[] =
    "Enable dynamically loading fields for address input";
const char kAutofillEnableDynamicallyLoadingFieldsForAddressInputDescription[] =
    "When enabled, the address fields for input would be dynamically loaded "
    "based on the country value";

const char kAutofillEnableMerchantDomainInUnmaskCardRequestName[] =
    "Enable sending merchant domain in server card unmask requests";
const char kAutofillEnableMerchantDomainInUnmaskCardRequestDescription[] =
    "When enabled, requests to unmask cards will include a top-level "
    "merchant_domain parameter populated with the last origin of the main "
    "frame.";

const char kAutofillEnablePaymentsMandatoryReauthName[] =
    "Enable mandatory re-auth for payments autofill";
const char kAutofillEnablePaymentsMandatoryReauthDescription[] =
    "When this is enabled, in use-cases where we would not have triggered any "
    "user-visible authentication to autofill payment methods, we will trigger "
    "a device authentication.";

const char kAutofillEnablePrefetchingRiskDataForRetrievalName[] =
    "Enable prefetching of risk data during payments autofill retrieval";
const char kAutofillEnablePrefetchingRiskDataForRetrievalDescription[] =
    "When enabled, risk data is prefetched during payments autofill flows "
    "to reduce user-perceived latency.";

const char kAutofillEnableRankingFormulaAddressProfilesName[] =
    "Enable new Autofill suggestion ranking formula for address profiles";
const char kAutofillEnableRankingFormulaAddressProfilesDescription[] =
    "When enabled, Autofill will use a new ranking formula to rank Autofill "
    "address profile suggestions.";

const char kAutofillEnableRankingFormulaCreditCardsName[] =
    "Enable new Autofill suggestion ranking formula for credit cards";
const char kAutofillEnableRankingFormulaCreditCardsDescription[] =
    "When enabled, Autofill will use a new ranking formula to rank Autofill "
    "data model credit card suggestions.";

const char kAutofillEnableRemadeDownstreamMetricsName[] =
    "Enable remade Autofill Downstream metrics logging";
const char kAutofillEnableRemadeDownstreamMetricsDescription[] =
    "When enabled, some extra metrics logging for Autofill Downstream will "
    "start.";

const char kAutofillEnableSupportForAdminLevel2Name[] =
    "Enables parsing and filling of administrative area level 2 fields";
const char kAutofillEnableSupportForAdminLevel2Description[] =
    "When enabled, Autofill will parse, save and fill administrative area "
    "level 2 fields in forms.";

const char kAutofillEnableSupportForBetweenStreetsName[] =
    "Enables parsing and filling of between street fields";
const char kAutofillEnableSupportForBetweenStreetsDescription[] =
    "When enabled, Autofill will parse, save and fill between street "
    "fields in forms.";

const char kAutofillEnableSupportForLandmarkName[] =
    "Enables parsing and filling of Landmark fields";
const char kAutofillEnableSupportForLandmarkDescription[] =
    "When enabled, Autofill will parse, save and fill landmark fields in "
    "forms.";

const char kAutofillEnableCardProductNameName[] =
    "Enable showing card product name";
const char kAutofillEnableCardProductNameDescription[] =
    "When enabled, card product name (instead of issuer network) will be shown "
    "in Payments UI.";

const char kAutofillEnableVirtualCardsName[] =
    "Enable virtual card enrollment and retrieval";
const char kAutofillEnableVirtualCardsDescription[] =
    "When enabled, virtual card enrollment and retrieval will be available on "
    "Bling.";

const char kAutofillEnableXHRSubmissionDetectionIOSName[] =
    "Enable XHR submission detection for Autofill";
const char kAutofillEnableXHRSubmissionDetectionIOSDescription[] =
    "When enabled, Chrome will detect forms submitted asynchronously (e.g. "
    "AJAX and XMLHttpRequest) for Autofill.";

const char kAutofillParseVcnCardOnFileStandaloneCvcFieldsName[] =
    "Parse standalone CVC fields for VCN card on file in forms";
const char kAutofillParseVcnCardOnFileStandaloneCvcFieldsDescription[] =
    "When enabled, Autofill will attempt to find standalone CVC fields for VCN "
    "card on file when parsing forms.";

const char kAutofillPruneSuggestionsName[] = "Autofill Prune Suggestions";
const char kAutofillPruneSuggestionsDescription[] =
    "Further limits the number of suggestions in the Autofill dropdown.";

const char kAutofillStickyInfobarName[] = "Sticky Autofill Infobar";
const char kAutofillStickyInfobarDescription[] =
    "Makes the Address Infobar sticky to only dismiss on navigation from user "
    "gesture.";

const char kAutofillSuggestServerCardInsteadOfLocalCardName[] =
    "Suggest Server card instead of Local card for deduped cards";
const char kAutofillSuggestServerCardInsteadOfLocalCardDescription[] =
    "When enabled, Autofill suggestions that consist of a local and server "
    "version of the same card will attempt to fill the server card upon "
    "selection instead of the local card.";

const char kAutofillUpdateChromeSettingsLinkToGPayWebName[] =
    "Update Chrome Settings Link to GPay Web";
const char kAutofillUpdateChromeSettingsLinkToGPayWebDescription[] =
    "When enabled, Chrome Settings link directs to GPay Web rather than "
    "Payments Center for payment methods management.";

const char kAutofillUseRendererIDsName[] =
    "Autofill logic uses unqiue renderer IDs";
const char kAutofillUseRendererIDsDescription[] =
    "When enabled, Autofill logic uses unique numeric renderer IDs instead "
    "of string form and field identifiers in form filling logic.";

const char kAutofillUseTwoDotsForLastFourDigitsName[] =
    "Autofill uses two '•' instead of four";
const char kAutofillUseTwoDotsForLastFourDigitsDescription[] =
    "When enabled, Autofill surfaces will show two '•' characters instead of "
    "four when displaying the last four digits of a card number";

const char kBottomOmniboxDefaultSettingName[] =
    "Bottom Omnibox Default Setting";
const char kBottomOmniboxDefaultSettingDescription[] =
    "Changes the default setting of the omnibox position. If the user "
    "hasn't already changed the setting, changes the omnibox position to top "
    "or bottom of the screen on iPhone. The default is top omnibox.";

const char kBottomOmniboxPromoAppLaunchName[] =
    "Bottom omnibox promo app-launch";
const char kBottomOmniboxPromoAppLaunchDescription[] =
    "Enables the app-launch promo for the bottom omnibox.";

const char kBottomOmniboxPromoDefaultPositionName[] =
    "Bottom omnibox promo default position";
const char kBottomOmniboxPromoDefaultPositionDescription[] =
    "Changes the default omnibox position in the FRE and app-launch promos.";

const char kBottomOmniboxPromoFREName[] = "Bottom omnibox promo FRE";
const char kBottomOmniboxPromoFREDescription[] =
    "Enables the FRE promo for the bottom omnibox.";

const char kBottomOmniboxPromoRegionFilterName[] =
    "Bottom omnibox promo region filter";
const char kBottomOmniboxPromoRegionFilterDescription[] =
    "When enabled the bottom omnibox promo is limited to some regions.";

const char kSpotlightDonateNewIntentsName[] = "Donate New Spotlight Intents";
const char kSpotlightDonateNewIntentsDescription[] =
    "Donates relevant intents to Siri when corresponding features are used";

const char kBreakpadNoDelayInitialUploadName[] =
    "Remove delay on initial crash upload";
const char kBreakpadNoDelayInitialUploadDescription[] =
    "When enabled, the initial crash uploading will not be delayed. When "
    "disabled, initial upload is delayed until deferred initialization. This "
    "does not affect recovery mode.";

extern const char kAppleCalendarExperienceKitName[] =
    "Experience Kit Apple Calendar";
extern const char kAppleCalendarExperienceKitDescription[] =
    "When enabled, long pressing on dates will trigger Experience Kit Apple "
    "Calendar event handling.";

const char kContentPushNotificationsName[] = "Content Push Notifications";
const char kContentPushNotificationsDescription[] =
    "Enables the content push notifications.";

extern const char kContextualPanelForceShowEntrypointName[] =
    "Force show Contextual Panel entrypoint";
extern const char kContextualPanelForceShowEntrypointDescription[] =
    "When enabled, the Contextual Panel entrypoint will be shown regardless of "
    "appearance prerequisites.";

const char kContextualPanelName[] = "Contextual Panel";
const char kContextualPanelDescription[] =
    "Enables the contextual panel feature.";

const char kCredentialProviderPerformanceImprovementsName[] =
    "Credential Provider Performance Improvements";
const char kCredentialProviderPerformanceImprovementsDescription[] =
    "Enables a series of performance improvements for the Credential Provider "
    "Extension.";

extern const char kPhoneNumberName[] = "Phone number experience enable";
extern const char kPhoneNumberDescription[] =
    "When enabled, one tapping or long pressing on a phone number will trigger "
    "the phone number experience.";

const char kMeasurementsName[] = "Measurements experience enable";
const char kMeasurementsDescription[] =
    "When enabled, one tapping or long pressing on a measurement will trigger "
    "the measurement conversion experience.";

const char kEnableViewportIntentsName[] = "Viewport intent detection";
const char kEnableViewportIntentsDescription[] =
    "When enabled the intents are detected live as the viewport is moved "
    "around.";

const char kEnableExpKitTextClassifierDateName[] =
    "Date with Text Classifier in Experience Kit";
const char kEnableExpKitTextClassifierDateDescription[] =
    "When enabled, Experience Kit will use Text Classifier library in "
    "date detection on long presses.";

const char kEnableExpKitTextClassifierAddressName[] =
    "Address with Text Classifier in Experience Kit";
const char kEnableExpKitTextClassifierAddressDescription[] =
    "When enabled, Experience Kit will use Text Classifier library in "
    "address detection on long presses.";

const char kEnableExpKitTextClassifierPhoneNumberName[] =
    "Phone Number with Text Classifier in Experience Kit";
const char kEnableExpKitTextClassifierPhoneNumberDescription[] =
    "When enabled, Experience Kit will use Text Classifier library in "
    "phone number detection on long presses.";

const char kEnableExpKitTextClassifierEmailName[] =
    "Email with Text Classifier in Experience Kit";
const char kEnableExpKitTextClassifierEmailDescription[] =
    "When enabled, Experience Kit will use Text Classifier library in "
    "email detection on long presses.";

const char kEnableFamilyLinkControlsName[] = "Family Link parental controls";
const char kEnableFamilyLinkControlsDescription[] =
    "Enables parental controls from Family Link on supervised accounts "
    "signed-in to Chrome.";

extern const char kOneTapForMapsName[] = "Enable one Tap Experience for Maps";
extern const char kOneTapForMapsDescription[] =
    "Enables the one tap experience for maps experience kit.";

const char kEnablePopoutOmniboxIpadName[] = "Popout omnibox (iPad)";
const char kEnablePopoutOmniboxIpadDescription[] =
    "Make omnibox popup appear in a detached rounded rectangle below the "
    "omnibox.";

extern const char kMagicStackName[] = "Enable Magic Stack";
extern const char kMagicStackDescription[] =
    "When enabled, a Magic Stack will be shown in the Home surface displaying "
    "a variety of modules.";

const char kMinorModeRestrictionsForHistorySyncOptInName[] =
    "Minor Mode Restrictions For History Sync Opt In";
const char kMinorModeRestrictionsForHistorySyncOptInDescription[] =
    "When enabled, Chrome will present opt in screens for turning on History "
    "Sync depending on CanShowHistorySyncOptInsWithoutMinorModeRestrictions "
    "capability value. Otherwise, the opt-in screens are unrestricted.";

const char kCredentialProviderExtensionPromoName[] =
    "Enable the Credential Provider Extension promo.";
const char kCredentialProviderExtensionPromoDescription[] =
    "When enabled, Credential Provider Extension promo will be "
    "presented to eligible users.";

const char kDefaultBrowserVideoInSettingsName[] =
    "Show default browser video in settings experiment";
const char kDefaultBrowserVideoInSettingsDescription[] =
    "When enabled, default browser video will be displayed to user in "
    "settings.";

const char kDefaultBrowserIntentsShowSettingsName[] =
    "Default Browser Intents show settings";
const char kDefaultBrowserIntentsShowSettingsDescription[] =
    "When enabled, external apps can trigger the settings screen showing "
    "default browser tutorial.";

const char kDefaultBrowserPromoForceShowPromoName[] =
    "Skip default browser promo triggering criteria";
const char kDefaultBrowserPromoForceShowPromoDescription[] =
    "When enabled, the user will be able to chose which default browser promo "
    "will skip the triggering criteria. For this flag to effectively force a "
    "default browser promo, enable IPH_iOSPromoDefaultBrowser.";

const char kDefaultBrowserTriggerCriteriaExperimentName[] =
    "Show default browser promo trigger criteria experiment";
const char kDefaultBrowserTriggerCriteriaExperimentDescription[] =
    "When enabled, default browser promo will be displayed to user without "
    "matching all the trigger criteria.";

const char kDetectMainThreadFreezeName[] = "Detect freeze in the main thread.";
const char kDetectMainThreadFreezeDescription[] =
    "A crash report will be uploaded if the main thread is frozen more than "
    "the time specified by this flag.";

const char kDisableFullscreenScrollingName[] = "Disable fullscreen scrolling";
const char kDisableFullscreenScrollingDescription[] =
    "When this flag is enabled and a user scroll a web page, toolbars will "
    "stay extanded and the user will not enter in fullscreen mode.";

const char kDiscoverFeedSportCardName[] = "Sport card in Discover feed";
const char kDiscoverFeedSportCardDescription[] =
    "Enables the live sport card in the NTP's Discover feed";

const char kEnableColorLensAndVoiceIconsInHomeScreenWidgetName[] =
    "Enable color Lens and voice icons in home screen widget.";
const char kEnableColorLensAndVoiceIconsInHomeScreenWidgetDescription[] =
    "Shows the color icons, rather than the monochrome icons.";

const char kEnableDiscoverFeedTopSyncPromoName[] =
    "Enables the top of feed sync promo.";
const char kEnableDiscoverFeedTopSyncPromoDescription[] =
    "When enabled, a sync promotion will be presented to eligible users on top "
    "of the feed cards.";

const char kEnableFeedHeaderSettingsName[] =
    "Enables the feed header settings.";
const char kEnableFeedHeaderSettingsDescription[] =
    "When enabled, some UI elements of the feed header can be modified.";

const char kEnableFriendlierSafeBrowsingSettingsEnhancedProtectionName[] =
    "Enable friendlier safe browsing settings enhanced protection";
const char
    kEnableFriendlierSafeBrowsingSettingsEnhancedProtectionDescription[] =
        "Updates the text, layout, icons, and links on both the privacy guide "
        "and the security settings page.";

const char kEnableFriendlierSafeBrowsingSettingsStandardProtectionName[] =
    "Enable Friendlier Safe Browsing Settings for standard protection";
const char
    kEnableFriendlierSafeBrowsingSettingsStandardProtectionDescription[] =
        "Updates the text and layout on both the privacy guide and the "
        "security "
        "settings page.";

const char kDisableLensCameraName[] = "Disable Lens camera experience";
const char kDisableLensCameraDescription[] =
    "When enabled, the option use Lens to search for images from your device "
    "camera menu when Google is the selected search engine, accessible from "
    "the home screen widget, new tab page, and keyboard, is disabled.";

const char kEnableRedInterstitialFaceliftName[] = "Red interstitial facelift";
const char kEnableRedInterstitialFaceliftDescription[] =
    "Enables red interstitial facelift UI changes, including icon, string, and "
    "style changes.";

const char kEditPasswordsInSettingsName[] = "Edit passwords in settings";
const char kEditPasswordsInSettingsDescription[] =
    "Enables password editing in settings.";

const char kEnableAutofillAddressSavePromptName[] =
    "Autofill Address Save Prompts";
const char kEnableAutofillAddressSavePromptDescription[] =
    "Enable the Autofill address save prompts.";

const char kEnableCompromisedPasswordsMutingName[] =
    "Enable the muting of compromised passwords in the Password Manager";
const char kEnableCompromisedPasswordsMutingDescription[] =
    "Enable the compromised password alert mutings in Password Manager to be "
    "respected in the app.";

const char kEnableDiscoverFeedDiscoFeedEndpointName[] =
    "Enable discover feed discofeed";
const char kEnableDiscoverFeedDiscoFeedEndpointDescription[] =
    "Enable using the discofeed endpoint for the discover feed.";

const char kEnableFeedAblationName[] = "Enables Feed Ablation";
const char kEnableFeedAblationDescription[] =
    "If Enabled the Feed will be removed from the NTP";

const char kEnableFeedContainmentName[] = "Enables feed containment";
const char kEnableFeedContainmentDescription[] =
    "If enabled, the feed is contained in a module on the Home surface.";

const char kEnableFeedCardMenuSignInPromoName[] =
    "Enable Feed card menu sign-in promotion";
const char kEnableFeedCardMenuSignInPromoDescription[] =
    "Display a sign-in promotion UI when signed out users click on "
    "personalization options within the feed card menu.";

const char kEnableFollowIPHExpParamsName[] =
    "Enable Follow IPH Experiment Parameters";
const char kEnableFollowIPHExpParamsDescription[] =
    "Enable follow IPH experiment parameters.";

const char kEnableFollowUIUpdateName[] = "Enable the Follow UI Update";
const char kEnableFollowUIUpdateDescription[] =
    "Enable Follow UI Update for the Feed.";

const char kEnableiPadFeedGhostCardsName[] = "Enable ghost cards on iPad feeds";
const char kEnableiPadFeedGhostCardsDescription[] =
    "Enables ghost cards placeholder when feed is loading on iPads.";

const char kEnablePreferencesAccountStorageName[] =
    "Enable the account data storage for preferences for syncing users";
const char kEnablePreferencesAccountStorageDescription[] =
    "Enables storing preferences in a second, Gaia-account-scoped storage for "
    "syncing users";

const char kEnableReadingListAccountStorageName[] =
    "Enable Reading List Account Storage";
const char kEnableReadingListAccountStorageDescription[] =
    "Enable the reading list account storage.";

const char kEnableReadingListSignInPromoName[] =
    "Enable Reading List Sign-in promo";
const char kEnableReadingListSignInPromoDescription[] =
    "Enable the sign-in promo view in the reading list screen.";

const char kEnableSignedOutViewDemotionName[] =
    "Enable signed out user view demotion";
const char kEnableSignedOutViewDemotionDescription[] =
    "Enable signed out user view demotion to avoid repeated content for signed "
    "out users.";

const char kEnableStartupImprovementsName[] =
    "Enable startup latency improvements";
const char kEnableStartupImprovementsDescription[] =
    "Enables startup latency improvements to make startup faster.";

const char kEnableSuggestionsScrollingOnIPadName[] =
    "Enable omnibox suggestions scrolling on iPad";
const char kEnableSuggestionsScrollingOnIPadDescription[] =
    "Enable omnibox suggestions scrolling on iPad and disable suggestions "
    "hiding on keyboard dismissal.";


const char kEnableWebChannelsName[] = "Enable WebFeed";
const char kEnableWebChannelsDescription[] =
    "Enable folowing content from web and display Following feed on NTP based "
    "on sites that users followed.";

const char kEnhancedSafeBrowsingPromoName[] =
    "Enable Enhanced Safe Browsing Promos";
const char kEnhancedSafeBrowsingPromoDescription[] =
    "When enabled, the Enhanced Safe Browsing inline and infobar promos are "
    "displayed given certain preconditions are met.";

const char kFeedBackgroundRefreshName[] = "Enable feed background refresh";
const char kFeedBackgroundRefreshDescription[] =
    "Schedules a feed background refresh after some minimum period of time has "
    "passed after the last refresh.";

const char kForceStartupSigninPromoName[] = "Display the startup sign-in promo";
const char kForceStartupSigninPromoDescription[] =
    "When enabled, the startup sign-in promo is always displayed when starting "
    "Chrome.";

const char kFullscreenImprovementName[] = "Improve fullscreen";
const char kFullscreenImprovementDescription[] =
    "When enabled, fullscreen should have a better stability.";

const char kFullscreenPromosManagerSkipInternalLimitsName[] =
    "Fullscreen Promos Manager (Skip internal Impression Limits)";
const char kFullscreenPromosManagerSkipInternalLimitsDescription[] =
    "When enabled, the internal Impression Limits of the Promos Manager will "
    "be ignored; this is useful for local development.";

const char kFullscreenSmoothScrollingName[] = "Fullscreen Smooth Scrolling";
const char kFullscreenSmoothScrollingDescription[] =
    "When enabled, the web view's insets are updated for scoll events. If "
    "disabled, the the web view's frame are updated.";

const char kHttpsUpgradesName[] = "HTTPS Upgrades";
const char kHttpsUpgradesDescription[] =
    "When enabled, eligible navigations will automatically be upgraded to "
    "HTTPS.";

const char kIncognitoNtpRevampName[] = "Revamped Incognito New Tab Page";
const char kIncognitoNtpRevampDescription[] =
    "When enabled, Incognito new tab page will have an updated UI.";

const char kIndicateIdentityErrorInOverflowMenuName[] =
    "Indicate Identity Error in Overflow Menu";
const char kIndicateIdentityErrorInOverflowMenuDescription[] =
    "When enabled, the Overflow Menu indicates the identity error with an "
    "error badge on the Settings destination";

const char kInProductHelpDemoModeName[] = "In-Product Help Demo Mode";
const char kInProductHelpDemoModeDescription[] =
    "When enabled, in-product help promotions occur exactly once per cold "
    "start. Enabled causes all in-product help promotions to occur. Enabling "
    "an individual promotion causes that promotion but no other promotions to "
    "occur.";

const char kIOSBrowserEditMenuMetricsName[] = "Browser edit menu metrics";
const char kIOSBrowserEditMenuMetricsDescription[] =
    "Collect metrics for edit menu usage.";

const char kIOSDockingPromoName[] = "Docking Promo";
const char kIOSDockingPromoDescription[] =
    "When enabled, the user will be presented an animated, instructional "
    "promo showing how to move Chrome to their native iOS dock.";

const char kIOSEditMenuPartialTranslateName[] =
    "Enable partial translate in edit menu";
const char kIOSEditMenuPartialTranslateDescription[] =
    "Replace the Apple translate entry in the web edit menu to use Google "
    "Translate instead.";

extern const char kIOSEditMenuSearchWithName[] =
    "Enable Search with in edit menu";
extern const char kIOSEditMenuSearchWithDescription[] =
    "Add an entry to search the web selection with your default search engine.";

extern const char kIOSEditMenuHideSearchWebName[] =
    "Hides Search Web in edit menu";
extern const char kIOSEditMenuHideSearchWebDescription[] =
    "Hides the Search Web entry in edit menu.";

extern const char kIOSExternalActionURLsName[] = "iOS external action URLs";
extern const char kIOSExternalActionURLsDescription[] =
    "When enabled, the browser will support handling external action URLs.";

const char kIOSKeyboardAccessoryUpgradeName[] =
    "Enable the keyboard accessory upgrade on iOS";
const char kIOSKeyboardAccessoryUpgradeDescription[] =
    "When enabled, the upgraded keyboard accessory UI will be presented.";

const char kIOSMagicStackCollectionViewName[] =
    "Enable using a UICollectionView for the Magic Stack";
const char kIOSMagicStackCollectionViewDescription[] =
    "When enabled, the Magic Stack will be using a UICollectionView "
    "implementation";

const char kIOSParcelTrackingName[] = "Parcel Tracking";
const char kIOSParcelTrackingDescription[] =
    "When enabled, the user will be able to track their packages.";

const char kIOSPasswordAuthOnEntryV2Name[] =
    "Password Manager Auth on Entry V2";
const char kIOSPasswordAuthOnEntryV2Description[] =
    "Requires Local Authentication before showing saved credentials in "
    "Password Manager subpages.";

const char kIOSSaveToDriveName[] = "IOS Save to Drive";
const char kIOSSaveToDriveDescription[] =
    "Enables the Save to Drive feature on iOS.";

const char kIOSSaveToPhotosName[] = "IOS Save to Photos";
const char kIOSSaveToPhotosDescription[] =
    "Enables the Save to Photos feature on iOS.";

const char kIOSPasswordBottomSheetName[] = "IOS Password Manager Bottom Sheet";
const char kIOSPasswordBottomSheetDescription[] =
    "Enables the display of the password bottom sheet on IOS.";

const char kIOSPasswordBottomSheetAutofocusName[] =
    "IOS Password Manager Bottom Sheet Autofocus";
const char kIOSPasswordBottomSheetAutofocusDescription[] =
    "Enables triggering the password bottom sheet on autofocus on IOS.";

const char kSyncWebauthnCredentialsName[] = "Sync WebAuthn credentials";
const char kSyncWebauthnCredentialsDescription[] =
    "Allow syncing, managing, and displaying Google Password Manager WebAuthn "
    "credential ('passkey') metadata";

const char kIOSPasswordSignInUffName[] = "Password sign-in uff";
const char kIOSPasswordSignInUffDescription[] =
    "Enables filling the username in username-first sign-in flows.";

const char kNewTabPageFieldTrialName[] =
    "New tab page features that target new users";
const char kNewTabPageFieldTrialDescription[] =
    "Enables new tab page features that are available on first run for new "
    "Chrome iOS users.";

const char kIOSSharedHighlightingColorChangeName[] =
    "IOS Shared Highlighting color change";
const char kIOSSharedHighlightingColorChangeDescription[] =
    "Changes the Shared Highlighting color of the text fragment"
    "away from the default yellow in iOS. Works with #scroll-to-text-ios flag.";

const char kIOSSharedHighlightingAmpName[] = "Shared Highlighting on AMP pages";
const char kIOSSharedHighlightingAmpDescription[] =
    "Enables the Create Link option on AMP pages.";

const char kIOSSharedHighlightingV2Name[] = "Text Fragments UI improvements";
const char kIOSSharedHighlightingV2Description[] =
    "Enables improvements to text fragments UI, including a menu for removing "
    "or resharing a highlight.";

const char kIOSTipsNotificationsName[] = "Tips Notifications";
const char kIOSTipsNotificationsDescription[] =
    "Enables Notifications with content to help new users get the most out of "
    "the app.";

const char kIPHForSafariSwitcherName[] = "IPH for Safari Switcher";
const char kIPHForSafariSwitcherDescription[] =
    "Enables displaying IPH for users who are considered Safari Switcher";

const char kIPHiOSPullToRefreshFeatureName[] = "IPH for Pull Down To Refresh";
const char kIPHiOSPullToRefreshFeatureDescription[] =
    "Enables displaying the gesture IPH instructing users to pull down to "
    "refresh on the tab content view to new users.";

const char kIPHiOSSwipeBackForwardFeatureName[] = "IPH for Swipe Back/Forward";
const char kIPHiOSSwipeBackForwardFeatureDescription[] =
    "Enables displaying the gesture IPH instructing users to swipe back or "
    "forward on the currently opened website to navigate to the previous or "
    "next one visited.";

const char kIPHiOSSwipeToolbarToChangeTabFeatureName[] =
    "IPH for Swipe Toolbar To Change Tab";
const char kIPHiOSSwipeToolbarToChangeTabFeatureDescription[] =
    "Enables displaying the gesture IPH instructing users to swipe the toolbar "
    "to navigate to the previous or next tab in the tab list.";

const char kIPHiOSTabGridSwipeRightForIncognitoName[] =
    "IPH for Swipe Right for Incognito on Tab Grid";
const char kIPHiOSTabGridSwipeRightForIncognitoDescription[] =
    "Enables displaying the gesture IPH instructing users to swipe right on "
    "regular tab grid to view incognito tabs to new users.";

const char kLinkedServicesSettingIosName[] = "Linked Services Setting";
const char kLinkedServicesSettingIosDescription[] =
    "Add Linked Services Setting to the Sync Settings page.";

const char kLockBottomToolbarName[] = "Lock bottom toolbar";
const char kLockBottomToolbarDescription[] =
    "When enabled, the bottom toolbar will not get collapsed when scrolling "
    "into fullscreen mode.";

const char kMetrickitNonCrashReportName[] = "Metrickit non-crash reports";
const char kMetrickitNonCrashReportDescription[] =
    "Enables sending Metrickit reports for non crash type (hang, "
    "cpu-exception, diskwrite-exception)";

const char kMigrateSyncingUserToSignedInName[] =
    "Migrate syncing user to signed-in";
const char kMigrateSyncingUserToSignedInDescription[] =
    "Enables the migration of syncing users to the signed-in, non-syncing "
    "state.";

const char kModernTabStripName[] = "Modern TabStrip";
const char kModernTabStripDescription[] =
    "When enabled, the newly implemented tabstrip can be tested.";

const char kMostVisitedTilesHorizontalRenderGroupName[] =
    "MVTiles Horizontal Render Group";
const char kMostVisitedTilesHorizontalRenderGroupDescription[] =
    "When enabled, the MV tiles are represented as individual matches";

const char kNativeFindInPageName[] = "Native Find in Page";
const char kNativeFindInPageDescription[] =
    "When enabled, the JavaScript implementation of the Find in Page feature "
    "is replaced with a native implementation which also enables searching "
    "text in PDF files. Available for iOS 16 or later.";

const char kNewNTPOmniboxLayoutName[] = "New NTP Omnibox Layout";
const char kNewNTPOmniboxLayoutDescription[] =
    "Enables the new NTP omnibox layout with leading-edge aligned hint label "
    "and magnifying glass icon.";

const char kOverflowMenuCustomizationName[] = "Overflow Menu Customization";
const char kOverflowMenuCustomizationDescription[] =
    "Allow users to customize the order of the overflow menu";

const char kNTPViewHierarchyRepairName[] = "NTP View Hierarchy Repair";
const char kNTPViewHierarchyRepairDescription[] =
    "Checks if NTP view hierarchy is broken and fixes it if necessary.";

const char kOmniboxCompanyEntityIconAdjustmentName[] =
    "Omnibox Company Entity Icon Adjustment";
const char kOmniboxCompanyEntityIconAdjustmentDescription[] =
    "When enabled, company entity icons may be replaced based on the search "
    "suggestions and their corresponding order.";

const char kOmniboxGroupingFrameworkForZPSName[] =
    "Omnibox Grouping Framework for ZPS";
const char kOmniboxGroupingFrameworkForZPSDescription[] =
    "Enables an alternative grouping implementation for omnibox "
    "autocompletion.";

const char kOmniboxGroupingFrameworkForTypedSuggestionsName[] =
    "Omnibox Grouping Framework for Typed Suggestions";
const char kOmniboxGroupingFrameworkForTypedSuggestionsDescription[] =
    "Enables an alternative grouping implementation for omnibox "
    "autocompletion.";

const char kOmniboxHttpsUpgradesName[] = "Omnibox HTTPS upgrades";
const char kOmniboxHttpsUpgradesDescription[] =
    "Enables HTTPS upgrades for omnibox navigations typed without a scheme";

const char kOmniboxInspireMeName[] = "Omnibox Trending Queries";
const char kOmniboxInspireMeDescription[] =
    "When enabled, appends additional suggestions based on local trends and "
    "optionally extends the ZPS limit.";

const char kOmniboxInspireMeSignedOutName[] =
    "Omnibox Trending Queries For Signed-Out users";
const char kOmniboxInspireMeSignedOutDescription[] =
    "When enabled, appends additional suggestions based on local trends and "
    "optionally extends the ZPS limit (for signed out users).";

const char kOmniboxKeyboardPasteButtonName[] = "Omnibox keyboard paste button";
const char kOmniboxKeyboardPasteButtonDescription[] =
    "Enables paste button in the omnibox's keyboard accessory. Only available "
    "from iOS 16 onward.";

const char kOmniboxUIMaxAutocompleteMatchesName[] =
    "Omnibox UI Max Autocomplete Matches";
const char kOmniboxUIMaxAutocompleteMatchesDescription[] =
    "Changes the maximum number of autocomplete matches displayed in the "
    "Omnibox UI.";

const char kOmniboxMaxZPSMatchesName[] = "Omnibox Max ZPS Matches";
const char kOmniboxMaxZPSMatchesDescription[] =
    "Changes the maximum number of autocomplete matches displayed in the "
    "zero-prefix state in the omnibox (e.g. on NTP when tapped on OB).";

const char kOmniboxOnDeviceHeadSuggestionsIncognitoName[] =
    "Omnibox on device head suggestions (incognito only)";
const char kOmniboxOnDeviceHeadSuggestionsIncognitoDescription[] =
    "Shows Google head non personalized search suggestions provided by a "
    "compact on device model for incognito";

const char kOmniboxOnDeviceHeadSuggestionsNonIncognitoName[] =
    "Omnibox on device head suggestions (non-incognito only)";
const char kOmniboxOnDeviceHeadSuggestionsNonIncognitoDescription[] =
    "Shows Google head non personalized search suggestions provided by a "
    "compact on device model for non-incognito";

const char kOmniboxMaxURLMatchesName[] = "Omnibox Max URL matches";
const char kOmniboxMaxURLMatchesDescription[] =
    "Limit the number of URL suggestions in the omnibox. The omnibox will "
    "still display more than MaxURLMatches if there are no non-URL suggestions "
    "to replace them.";

const char kOmniboxNewImplementationName[] =
    "Use experimental omnibox textfield";
const char kOmniboxNewImplementationDescription[] =
    "Uses a textfield implementation that doesn't use UILabels internally";

const char kOmniboxLocalHistoryZeroSuggestBeyondNTPName[] =
    "Allow local history zero-prefix suggestions beyond NTP";
const char kOmniboxLocalHistoryZeroSuggestBeyondNTPDescription[] =
    "Enables local history zero-prefix suggestions in every context in which "
    "the remote zero-prefix suggestions are enabled.";

const char kOmniboxOnDeviceTailSuggestionsName[] =
    "Omnibox on device tail suggestions";
const char kOmniboxOnDeviceTailSuggestionsDescription[] =
    "Google tail non personalized search suggestions provided by a compact on "
    "device model.";

const char kOmniboxPopulateShortcutsDatabaseName[] =
    "Omnibox shortcuts database";
const char kOmniboxPopulateShortcutsDatabaseDescription[] =
    "Enables storing successful query/match in the omnibox shortcut database "
    "to provider better suggestions ranking.";

const char kOmniboxPopupRowContentConfigurationName[] =
    "Omnibox popup row content configuration";
const char kOmniboxPopupRowContentConfigurationDescription[] =
    "Enables the use of content configuration for the omnibox popup row.";

const char kOmniboxRichAutocompletionName[] =
    "Omnibox rich inline autocompletion";
const char kOmniboxRichAutocompletionDescription[] =
    "Enables omnibox rich inline autocompletion. Expands inline autocomplete "
    "to any type of input that users repeatedly use to get to specific URLs.";

extern const char kOmniboxSuggestionsRTLImprovementsName[] =
    "Omnibox Improved RTL Suggestion Layout";
extern const char kOmniboxSuggestionsRTLImprovementsDescription[] =
    "Improved layout for suggestions in right-to-left contexts";

const char kOmniboxZeroSuggestInMemoryCachingName[] =
    "Omnibox Zero Prefix Suggestion in-memory caching";
const char kOmniboxZeroSuggestInMemoryCachingDescription[] =
    "Enables in-memory caching of zero prefix suggestions.";

const char kOmniboxZeroSuggestPrefetchingName[] =
    "Omnibox Zero Prefix Suggestion Prefetching on NTP";
const char kOmniboxZeroSuggestPrefetchingDescription[] =
    "Enables prefetching of the zero prefix suggestions for eligible users "
    "on the New Tab page.";

const char kOmniboxZeroSuggestPrefetchingOnSRPName[] =
    "Omnibox Zero Prefix Suggestion Prefetching on SRP";
const char kOmniboxZeroSuggestPrefetchingOnSRPDescription[] =
    "Enables prefetching of the zero prefix suggestions for eligible users "
    "on the Search Results page.";

const char kOmniboxZeroSuggestPrefetchingOnWebName[] =
    "Omnibox Zero Prefix Suggestion Prefetching on the Web";
const char kOmniboxZeroSuggestPrefetchingOnWebDescription[] =
    "Enables prefetching of the zero prefix suggestions for eligible users "
    "on the Web (i.e. non-NTP and non-SRP URLs).";

const char kOmniboxColorIconsName[] = "Enable color icons in the Omnibox";
const char kOmniboxColorIconsDescription[] =
    "When enabled, displays color microphone and Lens icons in the omnibox.";

const char kOnlyAccessClipboardAsyncName[] =
    "Only access the clipboard asynchronously";
const char kOnlyAccessClipboardAsyncDescription[] =
    "Only accesses the clipboard asynchronously.";

const char kOptimizationGuideDebugLogsName[] =
    "Enable optimization guide debug logs";
const char kOptimizationGuideDebugLogsDescription[] =
    "Enables the optimization guide to log and save debug messages that can be "
    "shown in the internals page.";

const char kOptimizationGuidePushNotificationClientName[] =
    "Enable optimization guide push notification client";
const char kOptimizationGuidePushNotificationClientDescription[] =
    "Enables the client that handles incoming push notifications on behalf of "
    "the optimization guide.";

const char kPageContentAnnotationsName[] = "Page content annotations";
const char kPageContentAnnotationsDescription[] =
    "Enables page content to be annotated on-device.";

const char kPageContentAnnotationsPersistSalientImageMetadataName[] =
    "Page content annotations - Persist salient image metadata";
const char kPageContentAnnotationsPersistSalientImageMetadataDescription[] =
    "Enables salient image metadata per page load to be persisted on-device.";

const char kPageContentAnnotationsRemotePageMetadataName[] =
    "Page content annotations - Remote page metadata";
const char kPageContentAnnotationsRemotePageMetadataDescription[] =
    "Enables fetching of page load metadata to be persisted on-device.";

const char kPageVisibilityPageContentAnnotationsName[] =
    "Page visibility content annotations";
const char kPageVisibilityPageContentAnnotationsDescription[] =
    "Enables annotating the page visibility model for each page load "
    "on-device.";

const char kPasswordReuseDetectionName[] =
    "PhishGuard password reuse detection";
const char kPasswordReuseDetectionDescription[] =
    "Displays warning when user types or pastes a saved password into a "
    "phishing website.";

const char kPasswordSharingName[] = "Enables password sharing";
const char kPasswordSharingDescription[] =
    "Enables password sharing between members of the same family.";

const char kEnablePolicyTestPageName[] =
    "Enable access to the policy test page";
const char kEnablePolicyTestPageDescription[] =
    "When enabled, allows the policy test page to be accessed at "
    "chrome://policy/test.";

const char kPrivacyGuideIosName[] = "Privacy Guide on iOS";
const char kPrivacyGuideIosDescription[] =
    "Shows a new subpage in Settings that helps the user to review various "
    "privacy settings.";

const char kIdleTimeoutPoliciesName[] =
    "IdleTimeout and IdleTimeoutActions Policies";
const char kIdleTimeoutPoliciesDescription[] =
    "Enable IdleTimeout and IdleTimeoutActions enterprise policies.";

const char kIPHPriceNotificationsWhileBrowsingName[] =
    "Price Tracking IPH Display";
const char kIPHPriceNotificationsWhileBrowsingDescription[] =
    "Displays the Price Tracking IPH when the user navigates to a "
    "product "
    "webpage that supports price tracking.";

const char kNotificationSettingsMenuItemName[] =
    "Notification Settings Menu Item";
const char kNotificationSettingsMenuItemDescription[] =
    "Displays the menu item for the notification controls inside the chrome "
    "settings UI.";

const char kRemoveExcessNTPsExperimentName[] = "Remove extra New Tab Pages";
const char kRemoveExcessNTPsExperimentDescription[] =
    "When enabled, extra tabs with the New Tab Page open and no navigation "
    "history will be removed.";

const char kRemoveOldWebStateRestoreName[] = "Remove Old WebState Restoration";
const char kRemoveOldWebStateRestoreDescription[] =
    "When enabled, skips "
    "attempting to restore WebState navigation history using "
    "session_restore.html.";

const char kRevampPageInfoIosName[] = "Revamp Page Info";
const char kRevampPageInfoIosDescription[] =
    "Revamps Page Info to add two new sections, AboutThisPage and Last "
    "Visited.";

const char kSafeBrowsingAvailableName[] = "Make Safe Browsing available";
const char kSafeBrowsingAvailableDescription[] =
    "When enabled, navigation URLs are compared to Safe Browsing blocklists, "
    "subject to an opt-out preference.";

const char kSafeBrowsingRealTimeLookupName[] = "Enable real-time Safe Browsing";
const char kSafeBrowsingRealTimeLookupDescription[] =
    "When enabled, navigation URLs are checked using real-time queries to Safe "
    "Browsing servers, subject to an opt-in preference.";

const char kSafetyCheckMagicStackName[] = "Enable Safety Check (Magic Stack)";
const char kSafetyCheckMagicStackDescription[] =
    "When enabled, the Safety Check module will be displayed in the Magic "
    "Stack.";

const char kScreenTimeIntegrationName[] = "Enables ScreenTime Integration";
const char kScreenTimeIntegrationDescription[] =
    "Enables integration with ScreenTime in iOS 14.0 and above.";

const char kSegmentationPlatformIosModuleRankerName[] =
    "Enable Magic Stack Segmentation Ranking";
const char kSegmentationPlatformIosModuleRankerDescription[] =
    "Enables the Segmentation platform to rank Magic Stack modules";

const char kSendUmaOverAnyNetwork[] =
    "Send UMA data over any network available.";
const char kSendUmaOverAnyNetworkDescription[] =
    "When enabled, will send UMA data over either WiFi or cellular by default.";

const char kSharedHighlightingIOSName[] = "Enable Shared Highlighting features";
const char kSharedHighlightingIOSDescription[] =
    "Adds a Link to Text option in the Edit Menu which generates URLs with a "
    "text fragment.";

const char kShareInWebContextMenuIOSName[] = "Share in web context menu";
const char kShareInWebContextMenuIOSDescription[] =
    "Enables the Share button in the web context menu in iOS 16.0 and above.";

const char kShowAutofillTypePredictionsName[] = "Show Autofill predictions";
const char kShowAutofillTypePredictionsDescription[] =
    "Annotates web forms with Autofill field type predictions as placeholder "
    "text.";

const char kSpotlightOpenTabsSourceName[] = "Show Open local tabs in Spotlight";
const char kSpotlightOpenTabsSourceDescription[] =
    "Donate local open tabs items to iOS Search Engine Spotlight";

const char kSpotlightReadingListSourceName[] = "Show Reading List in Spotlight";
const char kSpotlightReadingListSourceDescription[] =
    "Donate Reading List items to iOS Search Engine Spotlight";

const char kSyncRememberCustomPassphraseAfterSignoutName[] =
    "Remember custom passphrase after sign-out";
const char kSyncRememberCustomPassphraseAfterSignoutDescription[] =
    "Remember custom passphrase after sign-out, instead of asking the user to "
    "re-enter it after every sign-in.";

const char kSyncSandboxName[] = "Use Chrome Sync sandbox";
const char kSyncSandboxDescription[] =
    "Connects to the testing server for Chrome Sync.";

const char kSyncSessionOnVisibilityChangedName[] =
    "Sync session when tab visibility changes";
const char kSyncSessionOnVisibilityChangedDescription[] =
    "This flag enables session syncing when the visibility of a tab changes.";

const char kSyncSegmentsDataName[] = "Use synced segments data";
const char kSyncSegmentsDataDescription[] =
    "Enables history's segments to include foreign visits from syncing "
    "devices.";

const char kStartSurfaceName[] = "Start Surface";
const char kStartSurfaceDescription[] =
    "Enable showing the Start Surface when launching Chrome via clicking the "
    "icon or the app switcher.";

const char kDownloadServiceForegroundSessionName[] =
    "Download service foreground download";
const char kDownloadServiceForegroundSessionDescription[] =
    "Enable download service to download in app foreground only";

const char kTFLiteLanguageDetectionName[] = "TFLite-based Language Detection";
const char kTFLiteLanguageDetectionDescription[] =
    "Uses TFLite for language detection in place of CLD3";

const char kTFLiteLanguageDetectionIgnoreName[] =
    "Ignore TFLite-based Language Detection";
const char kTFLiteLanguageDetectionIgnoreDescription[] =
    "Computes the TFLite language detection but ignore the result and uses the "
    "CLD3 detection instead.";

const char kThemeColorInTopToolbarName[] = "Top toolbar use page's theme color";
const char kThemeColorInTopToolbarDescription[] =
    "When enabled with bottom omnibox, the top toolbar background color is the "
    "page's theme color. Disabled when a dynamic color flag is enabled.";

const char kIOSHideFeedWithSearchChoiceName[] = "Hide Feed with Search Choice";
const char kIOSHideFeedWithSearchChoiceDescription[] =
    "When enabled, the feed and feed header are hidden depending on the "
    "Search Engine setting.";

const char kIOSLargeFakeboxName[] = "Enable Large Fakebox on Home";
const char kIOSLargeFakeboxDescription[] =
    "When enabled, the Fakebox on Home appears larger and has an updated "
    "design.";

const char kEnableLensInOmniboxCopiedImageName[] =
    "Enable Google Lens in the Omnibox for Copied Images";
const char kEnableLensInOmniboxCopiedImageDescription[] =
    "When enabled, use Lens to search images from your device clipboard "
    "when Google is the selected search engine, accessible from the omnibox or "
    "popup menu.";

const char kEnableSessionSerializationOptimizationsName[] =
    "Session Serialization Optimization";
const char kEnableSessionSerializationOptimizationsDescription[] =
    "Enables the use of multiple separate files to save the session state "
    "and the ability to load only the minimum amount of data when restoring "
    "the session from disk.";

const char kTabGridAlwaysBounceName[] =
    "Let the tab grid bounce even if the content fits the screen";
const char kTabGridAlwaysBounceDescription[] =
    "When enabled, the Tab Grid bounces (aka overscrolls) even if all tabs are "
    "fully visible on screen.";

const char kTabGridCompositionalLayoutName[] =
    "Enable tab grid with the new compositional layout";
const char kTabGridCompositionalLayoutDescription[] =
    "When enabled, the Tab Grid uses the new compositional layout. Items sizes "
    "are different and more dynamic than before.";

const char kTabGridRefactoringName[] = "Enable tab grid refactoring";
const char kTabGridRefactoringDescription[] =
    "When enabled, the Tab Grid uses the refactored version, it should not "
    "have any visual difference nor different feature with the legacy one.";

const char kTabGridNewTransitionsName[] = "Enable new TabGrid transitions";
const char kTabGridNewTransitionsDescription[] =
    "When enabled, the new Tab Grid to Browser (and vice versa) transitions"
    "are used.";

const char kTabGroupsInGridName[] = "Enable Tab Groups in grid";
const char kTabGroupsInGridDescription[] =
    "When enabled, tab groups can be created from the grid.";

const char kTabGroupsIPadName[] = "Enable Tab Groups on iPad";
const char kTabGroupsIPadDescription[] =
    "When enabled, if tab-groups-in-grid is enabled, tab group can be created "
    "on iPad.";

const char kTabInactivityThresholdName[] = "Change Tab inactivity threshold";
const char kTabInactivityThresholdDescription[] =
    "When enabled, the tabs older than the threshold are considered inactive "
    "and set aside in the Inactive Tabs section of the TabGrid."
    "IMPORTANT: If you ever used the in-app settings for Inactive Tabs, this "
    "flag is never read again.";

const char kTabPickupMinimumDelayName[] =
    "Add delay between tab pickup banners";
const char kTabPickupMinimumDelayDescription[] =
    "When enabled, adds a 2h delay between the presentation of two tab pickup "
    "banners.";

const char kTabPickupThresholdName[] = "Enable and change tab pickup threshold";
const char kTabPickupThresholdDescription[] =
    "When enabled, an infobar will be displayed when the latest tab used from "
    "another device is yougner than the threshold.";

const char kTabResumptionName[] = "Enable Tab Resumption";
const char kTabResumptionDescription[] =
    "When enabled, offer users with a quick shortcut to resume the last synced "
    "tab from another device.";

const char kUndoMigrationOfSyncingUserToSignedInName[] =
    "Undo the migration of syncing users to signed-in";
const char kUndoMigrationOfSyncingUserToSignedInDescription[] =
    "Enables the reverse-migration of syncing users who were previously "
    "migrated to the signed-in, non-syncing state.";

const char kUnifiedBookmarkModelName[] = "Use unified bookmark model";
const char kUnifiedBookmarkModelDescription[] =
    "When enabled, all bookmarks are represented in a single BookmarkModel "
    "object per BrowserState, instead of using two instances.";

const char kUseLoadSimulatedRequestForOfflinePageName[] =
    "Use loadSimulatedRequest:responseHTMLString: when displaying offline "
    "pages";
const char kUseLoadSimulatedRequestForOfflinePageDescription[] =
    "When enabled, the offline pages uses the iOS 15 "
    "loadSimulatedRequest:responseHTMLString: API";

const char kWaitThresholdMillisecondsForCapabilitiesApiName[] =
    "Maximum wait time (in seconds) for a response from the Account "
    "Capabilities API";
const char kWaitThresholdMillisecondsForCapabilitiesApiDescription[] =
    "Used for testing purposes to test waiting thresholds in dev.";

const char kWalletServiceUseSandboxName[] = "Use Google Payments sandbox";
const char kWalletServiceUseSandboxDescription[] =
    "Uses the sandbox service for Google Payments API calls.";

const char kWebFeedFeedbackRerouteName[] =
    "Send discover feed feedback to a updated destination";
const char kWebFeedFeedbackRerouteDescription[] =
    "Directs discover feed feedback to a new target for better handling of the"
    "feedback reports.";

const char kWebPageDefaultZoomFromDynamicTypeName[] =
    "Use dynamic type size for default text zoom level";
const char kWebPageDefaultZoomFromDynamicTypeDescription[] =
    "When enabled, the default text zoom level for a website comes from the "
    "current dynamic type setting.";

const char kWebPageAlternativeTextZoomName[] =
    "Use different method for zooming web pages";
const char kWebPageAlternativeTextZoomDescription[] =
    "When enabled, switches the method used to zoom web pages.";

const char kWebPageTextZoomIPadName[] = "Enable text zoom on iPad";
const char kWebPageTextZoomIPadDescription[] =
    "When enabled, text zoom works again on iPad";

// Please insert your name/description above in alphabetical order.

}  // namespace flag_descriptions
