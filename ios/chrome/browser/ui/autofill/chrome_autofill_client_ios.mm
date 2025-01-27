// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/chrome_autofill_client_ios.h"

#import <utility>
#import <vector>

#import "base/check.h"
#import "base/functional/bind.h"
#import "base/functional/callback.h"
#import "base/memory/ptr_util.h"
#import "base/notreached.h"
#import "base/strings/string_util.h"
#import "base/strings/sys_string_conversions.h"
#import "base/strings/utf_string_conversions.h"
#import "components/autofill/core/browser/autofill_plus_address_delegate.h"
#import "components/autofill/core/browser/autofill_save_update_address_profile_delegate_ios.h"
#import "components/autofill/core/browser/form_data_importer.h"
#import "components/autofill/core/browser/logging/log_manager.h"
#import "components/autofill/core/browser/payments/autofill_credit_card_filling_infobar_delegate_mobile.h"
#import "components/autofill/core/browser/payments/autofill_save_card_delegate.h"
#import "components/autofill/core/browser/payments/autofill_save_card_infobar_delegate_mobile.h"
#import "components/autofill/core/browser/payments/autofill_save_card_ui_info.h"
#import "components/autofill/core/browser/payments/credit_card_cvc_authenticator.h"
#import "components/autofill/core/browser/payments/credit_card_otp_authenticator.h"
#import "components/autofill/core/browser/payments/credit_card_risk_based_authenticator.h"
#import "components/autofill/core/browser/payments/payments_network_interface.h"
#import "components/autofill/core/browser/payments/virtual_card_enroll_metrics_logger.h"
#import "components/autofill/core/browser/payments/virtual_card_enrollment_manager.h"
#import "components/autofill/core/browser/ui/payments/card_unmask_authentication_selection_dialog_controller_impl.h"
#import "components/autofill/core/browser/ui/payments/virtual_card_enroll_ui_model.h"
#import "components/autofill/core/browser/ui/popup_item_ids.h"
#import "components/autofill/core/common/autofill_features.h"
#import "components/autofill/core/common/autofill_prefs.h"
#import "components/autofill/ios/browser/autofill_util.h"
#import "components/infobars/core/infobar.h"
#import "components/infobars/core/infobar_manager.h"
#import "components/keyed_service/core/service_access_type.h"
#import "components/password_manager/core/common/password_manager_pref_names.h"
#import "components/plus_addresses/plus_address_service.h"
#import "components/security_state/ios/security_state_utils.h"
#import "components/sync/service/sync_service.h"
#import "components/translate/core/browser/translate_manager.h"
#import "components/ukm/ios/ukm_url_recorder.h"
#import "components/variations/service/variations_service.h"
#import "ios/chrome/browser/autofill/model/address_normalizer_factory.h"
#import "ios/chrome/browser/autofill/model/autocomplete_history_manager_factory.h"
#import "ios/chrome/browser/autofill/model/autofill_log_router_factory.h"
#import "ios/chrome/browser/autofill/model/bottom_sheet/autofill_bottom_sheet_tab_helper.h"
#import "ios/chrome/browser/autofill/model/credit_card/autofill_save_card_infobar_delegate_ios.h"
#import "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#import "ios/chrome/browser/autofill/model/strike_database_factory.h"
#import "ios/chrome/browser/device_reauth/ios_device_authenticator.h"
#import "ios/chrome/browser/device_reauth/ios_device_authenticator_factory.h"
#import "ios/chrome/browser/history/model/history_service_factory.h"
#import "ios/chrome/browser/infobars/model/infobar_ios.h"
#import "ios/chrome/browser/infobars/model/infobar_utils.h"
#import "ios/chrome/browser/passwords/model/password_tab_helper.h"
#import "ios/chrome/browser/plus_addresses/model/plus_address_service_factory.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/public/commands/autofill_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/signin/model/authentication_service.h"
#import "ios/chrome/browser/signin/model/authentication_service_factory.h"
#import "ios/chrome/browser/signin/model/identity_manager_factory.h"
#import "ios/chrome/browser/sync/model/sync_service_factory.h"
#import "ios/chrome/browser/translate/model/chrome_ios_translate_client.h"
#import "ios/chrome/browser/ui/autofill/card_expiration_date_fix_flow_view_bridge.h"
#import "ios/chrome/browser/ui/autofill/card_name_fix_flow_view_bridge.h"
#import "ios/chrome/browser/ui/autofill/scoped_autofill_payment_reauth_module_override.h"
#import "ios/chrome/browser/webdata_services/model/web_data_service_factory.h"
#import "ios/chrome/common/channel_info.h"
#import "ios/web/public/web_state.h"
#import "services/network/public/cpp/shared_url_loader_factory.h"
#import "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

namespace autofill {

namespace {

// Creates and returns an infobar for saving credit cards.
std::unique_ptr<infobars::InfoBar> CreateSaveCardInfoBarMobile(
    std::unique_ptr<AutofillSaveCardInfoBarDelegateIOS> delegate) {
  return std::make_unique<InfoBarIOS>(InfobarType::kInfobarTypeSaveCard,
                                      std::move(delegate));
}

}  // namespace

ChromeAutofillClientIOS::ChromeAutofillClientIOS(
    ChromeBrowserState* browser_state,
    web::WebState* web_state,
    infobars::InfoBarManager* infobar_manager,
    id<AutofillClientIOSBridge> bridge)
    : pref_service_(browser_state->GetPrefs()),
      sync_service_(SyncServiceFactory::GetForBrowserState(browser_state)),
      personal_data_manager_(PersonalDataManagerFactory::GetForBrowserState(
          browser_state->GetOriginalChromeBrowserState())),
      autocomplete_history_manager_(
          AutocompleteHistoryManagerFactory::GetForBrowserState(browser_state)),
      browser_state_(browser_state),
      web_state_(web_state),
      bridge_(bridge),
      identity_manager_(IdentityManagerFactory::GetForBrowserState(
          browser_state->GetOriginalChromeBrowserState())),
      form_data_importer_(std::make_unique<FormDataImporter>(
          this,
          personal_data_manager_,
          ios::HistoryServiceFactory::GetForBrowserState(
              browser_state,
              ServiceAccessType::EXPLICIT_ACCESS),
          GetApplicationContext()->GetApplicationLocale())),
      infobar_manager_(infobar_manager),
      // TODO(crbug.com/928595): Replace the closure with a callback to the
      // renderer that indicates if log messages should be sent from the
      // renderer.
      log_manager_(LogManager::Create(
          AutofillLogRouterFactory::GetForBrowserState(browser_state),
          base::RepeatingClosure())) {}

ChromeAutofillClientIOS::~ChromeAutofillClientIOS() {
  HideAutofillPopup(PopupHidingReason::kTabGone);
}

void ChromeAutofillClientIOS::SetBaseViewController(
    UIViewController* base_view_controller) {
  base_view_controller_ = base_view_controller;
}

version_info::Channel ChromeAutofillClientIOS::GetChannel() const {
  return ::GetChannel();
}

bool ChromeAutofillClientIOS::IsOffTheRecord() const {
  return web_state_->GetBrowserState()->IsOffTheRecord();
}

scoped_refptr<network::SharedURLLoaderFactory>
ChromeAutofillClientIOS::GetURLLoaderFactory() {
  return base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
      web_state_->GetBrowserState()->GetURLLoaderFactory());
}

AutofillCrowdsourcingManager*
ChromeAutofillClientIOS::GetCrowdsourcingManager() {
  if (!crowdsourcing_manager_) {
    // Lazy initialization to avoid virtual function calls in the constructor.
    crowdsourcing_manager_ = std::make_unique<AutofillCrowdsourcingManager>(
        this, GetChannel(), GetLogManager());
  }
  return crowdsourcing_manager_.get();
}

PersonalDataManager* ChromeAutofillClientIOS::GetPersonalDataManager() {
  return personal_data_manager_;
}

AutocompleteHistoryManager*
ChromeAutofillClientIOS::GetAutocompleteHistoryManager() {
  return autocomplete_history_manager_;
}

CreditCardCvcAuthenticator* ChromeAutofillClientIOS::GetCvcAuthenticator() {
  if (!cvc_authenticator_)
    cvc_authenticator_ = std::make_unique<CreditCardCvcAuthenticator>(this);
  return cvc_authenticator_.get();
}

CreditCardOtpAuthenticator* ChromeAutofillClientIOS::GetOtpAuthenticator() {
  if (!otp_authenticator_)
    otp_authenticator_ = std::make_unique<CreditCardOtpAuthenticator>(this);
  return otp_authenticator_.get();
}

CreditCardRiskBasedAuthenticator*
ChromeAutofillClientIOS::GetRiskBasedAuthenticator() {
  if (!risk_based_authenticator_) {
    risk_based_authenticator_ =
        std::make_unique<CreditCardRiskBasedAuthenticator>(this);
  }
  return risk_based_authenticator_.get();
}

PrefService* ChromeAutofillClientIOS::GetPrefs() {
  return const_cast<PrefService*>(std::as_const(*this).GetPrefs());
}

const PrefService* ChromeAutofillClientIOS::GetPrefs() const {
  return pref_service_;
}

syncer::SyncService* ChromeAutofillClientIOS::GetSyncService() {
  return sync_service_;
}

signin::IdentityManager* ChromeAutofillClientIOS::GetIdentityManager() {
  return identity_manager_;
}

FormDataImporter* ChromeAutofillClientIOS::GetFormDataImporter() {
  return form_data_importer_.get();
}

payments::IOSChromePaymentsAutofillClient*
ChromeAutofillClientIOS::GetPaymentsAutofillClient() {
  if (!payments_autofill_client_) {
    payments_autofill_client_ =
        std::make_unique<payments::IOSChromePaymentsAutofillClient>(
            this, browser_state_);
  }

  return payments_autofill_client_.get();
}

StrikeDatabase* ChromeAutofillClientIOS::GetStrikeDatabase() {
  return StrikeDatabaseFactory::GetForBrowserState(
      browser_state_->GetOriginalChromeBrowserState());
}

ukm::UkmRecorder* ChromeAutofillClientIOS::GetUkmRecorder() {
  return GetApplicationContext()->GetUkmRecorder();
}

ukm::SourceId ChromeAutofillClientIOS::GetUkmSourceId() {
  return ukm::GetSourceIdForWebStateDocument(web_state_);
}

AddressNormalizer* ChromeAutofillClientIOS::GetAddressNormalizer() {
  return AddressNormalizerFactory::GetInstance();
}

const GURL& ChromeAutofillClientIOS::GetLastCommittedPrimaryMainFrameURL()
    const {
  return web_state_->GetLastCommittedURL();
}

url::Origin ChromeAutofillClientIOS::GetLastCommittedPrimaryMainFrameOrigin()
    const {
  return url::Origin::Create(GetLastCommittedPrimaryMainFrameURL());
}

security_state::SecurityLevel
ChromeAutofillClientIOS::GetSecurityLevelForUmaHistograms() {
  return security_state::GetSecurityLevelForWebState(web_state_);
}

const translate::LanguageState* ChromeAutofillClientIOS::GetLanguageState() {
  // TODO(crbug.com/912597): iOS vs other platforms extracts language from
  // the top level frame vs whatever frame directly holds the form.
  auto* translate_client = ChromeIOSTranslateClient::FromWebState(web_state_);
  if (translate_client) {
    auto* translate_manager = translate_client->GetTranslateManager();
    if (translate_manager)
      return translate_manager->GetLanguageState();
  }
  return nullptr;
}

translate::TranslateDriver* ChromeAutofillClientIOS::GetTranslateDriver() {
  auto* translate_client = ChromeIOSTranslateClient::FromWebState(web_state_);
  if (translate_client) {
    return translate_client->GetTranslateDriver();
  }
  return nullptr;
}

GeoIpCountryCode ChromeAutofillClientIOS::GetVariationConfigCountryCode()
    const {
  variations::VariationsService* variation_service =
      GetApplicationContext()->GetVariationsService();
  // Retrieves the country code from variation service and converts it to upper
  // case.
  return GeoIpCountryCode(
      variation_service
          ? base::ToUpperASCII(variation_service->GetLatestCountry())
          : std::string());
}

void ChromeAutofillClientIOS::ShowAutofillSettings(
    FillingProduct main_filling_product) {
  NOTREACHED();
}

void ChromeAutofillClientIOS::ShowUnmaskAuthenticatorSelectionDialog(
    const std::vector<CardUnmaskChallengeOption>& challenge_options,
    base::OnceCallback<void(const std::string&)>
        confirm_unmask_challenge_option_callback,
    base::OnceClosure cancel_unmasking_closure) {
  AutofillBottomSheetTabHelper* bottomSheetTabHelper =
      AutofillBottomSheetTabHelper::FromWebState(web_state_);
  auto controller = std::make_unique<
      autofill::CardUnmaskAuthenticationSelectionDialogControllerImpl>(
      challenge_options, std::move(confirm_unmask_challenge_option_callback),
      std::move(cancel_unmasking_closure));
  card_unmask_authentication_selection_controller_ = controller->GetWeakPtr();
  bottomSheetTabHelper->ShowCardUnmaskAuthenticationSelection(
      std::move(controller));
}

void ChromeAutofillClientIOS::DismissUnmaskAuthenticatorSelectionDialog(
    bool server_success) {
  if (card_unmask_authentication_selection_controller_) {
    card_unmask_authentication_selection_controller_
        ->DismissDialogUponServerProcessedAuthenticationMethodRequest(
            server_success);
  }
}

payments::MandatoryReauthManager*
ChromeAutofillClientIOS::GetOrCreatePaymentsMandatoryReauthManager() {
  if (!payments_reauth_manager_) {
    payments_reauth_manager_ =
        std::make_unique<payments::MandatoryReauthManager>(this);
  }
  return payments_reauth_manager_.get();
}

void ChromeAutofillClientIOS::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    SaveCreditCardOptions options,
    LocalSaveCardPromptCallback callback) {
  DCHECK(options.show_prompt);
  infobar_manager_->AddInfoBar(CreateSaveCardInfoBarMobile(
      std::make_unique<AutofillSaveCardInfoBarDelegateIOS>(
          AutofillSaveCardUiInfo::CreateForLocalSave(options, card),
          std::make_unique<AutofillSaveCardDelegate>(std::move(callback),
                                                     options))));
}

void ChromeAutofillClientIOS::ConfirmAccountNameFixFlow(
    base::OnceCallback<void(const std::u16string&)> callback) {
  std::u16string account_name = base::UTF8ToUTF16(
      identity_manager_
          ->FindExtendedAccountInfo(identity_manager_->GetPrimaryAccountInfo(
              signin::ConsentLevel::kSignin))
          .full_name);

  card_name_fix_flow_controller_.Show(
      // CardNameFixFlowViewBridge manages its own lifetime, so
      // do not use std::unique_ptr<> here.
      new CardNameFixFlowViewBridge(&card_name_fix_flow_controller_,
                                    base_view_controller_),
      account_name, std::move(callback));
}

void ChromeAutofillClientIOS::ConfirmExpirationDateFixFlow(
    const CreditCard& card,
    base::OnceCallback<void(const std::u16string&, const std::u16string&)>
        callback) {
  card_expiration_date_fix_flow_controller_.Show(
      // CardExpirationDateFixFlowViewBridge manages its own lifetime,
      // so do not use std::unique_ptr<> here.
      new CardExpirationDateFixFlowViewBridge(
          &card_expiration_date_fix_flow_controller_, base_view_controller_),
      card, std::move(callback));
}

void ChromeAutofillClientIOS::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    const LegalMessageLines& legal_message_lines,
    SaveCreditCardOptions options,
    UploadSaveCardPromptCallback callback) {
  DCHECK(options.show_prompt);

  AccountInfo account_info = identity_manager_->FindExtendedAccountInfo(
      identity_manager_->GetPrimaryAccountInfo(signin::ConsentLevel::kSignin));
  infobar_manager_->AddInfoBar(CreateSaveCardInfoBarMobile(
      std::make_unique<AutofillSaveCardInfoBarDelegateIOS>(
          AutofillSaveCardUiInfo::CreateForUploadSave(
              options, card, legal_message_lines, account_info),
          std::make_unique<AutofillSaveCardDelegate>(std::move(callback),
                                                     options))));
}

void ChromeAutofillClientIOS::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    base::OnceClosure callback) {
  auto infobar_delegate =
      std::make_unique<AutofillCreditCardFillingInfoBarDelegateMobile>(
          card, std::move(callback));
  auto* raw_delegate = infobar_delegate.get();
  if (infobar_manager_->AddInfoBar(
          ::CreateConfirmInfoBar(std::move(infobar_delegate)))) {
    raw_delegate->set_was_shown();
  }
}

void ChromeAutofillClientIOS::ConfirmSaveAddressProfile(
    const AutofillProfile& profile,
    const AutofillProfile* original_profile,
    SaveAddressProfilePromptOptions options,
    AddressProfileSavePromptCallback callback) {
  // TODO(crbug.com/1167062): Respect SaveAddressProfilePromptOptions.
  for (infobars::InfoBar* infobar : infobar_manager_->infobars()) {
    AutofillSaveUpdateAddressProfileDelegateIOS* existing_delegate =
        AutofillSaveUpdateAddressProfileDelegateIOS::FromInfobarDelegate(
            infobar->delegate());

    if (existing_delegate) {
      if (existing_delegate->is_infobar_visible()) {
        // AutoDecline the new prompt if the existing prompt is visible.
        std::move(callback).Run(
            AutofillClient::AddressPromptUserDecision::kAutoDeclined, profile);
        return;
      } else {
        // If the existing prompt is not visible, it means that the user has
        // closed the prompt of the previous import process using the "Cancel"
        // button or it has been accepted by the user. A fallback icon is shown
        // for the user in the omnibox to get back to the prompt. If it is
        // already accepted by the user, the save button is disabled and the
        // saved data is shown to user. In both the cases, the original prompt
        // is replaced by the new one provided that the modal view of the
        // original infobar is not visible to the user.
        infobar_manager_->RemoveInfoBar(infobar);
        break;
      }
    }
  }

  auto delegate = std::make_unique<AutofillSaveUpdateAddressProfileDelegateIOS>(
      profile, original_profile, GetUserEmail(),
      GetApplicationContext()->GetApplicationLocale(), options,
      std::move(callback));

  infobar_manager_->AddInfoBar(std::make_unique<InfoBarIOS>(
      InfobarType::kInfobarTypeSaveAutofillAddressProfile,
      std::move(delegate)));
}

void ChromeAutofillClientIOS::ShowEditAddressProfileDialog(
    const AutofillProfile& profile,
    AddressProfileSavePromptCallback on_user_decision_callback) {
  NOTREACHED_NORETURN();
}

void ChromeAutofillClientIOS::ShowDeleteAddressProfileDialog(
    const AutofillProfile& profile,
    AddressProfileDeleteDialogCallback delete_dialog_callback) {
  NOTREACHED_NORETURN();
}

bool ChromeAutofillClientIOS::HasCreditCardScanFeature() const {
  return false;
}

void ChromeAutofillClientIOS::ScanCreditCard(CreditCardScanCallback callback) {
  NOTREACHED();
}

bool ChromeAutofillClientIOS::ShowTouchToFillCreditCard(
    base::WeakPtr<TouchToFillDelegate> delegate,
    base::span<const CreditCard> cards_to_suggest) {
  NOTREACHED();
  return false;
}

void ChromeAutofillClientIOS::HideTouchToFillCreditCard() {
  NOTREACHED();
}

void ChromeAutofillClientIOS::ShowAutofillPopup(
    const AutofillClient::PopupOpenArgs& open_args,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  [bridge_ showAutofillPopup:open_args.suggestions popupDelegate:delegate];
}

AutofillPlusAddressDelegate* ChromeAutofillClientIOS::GetPlusAddressDelegate() {
  return PlusAddressServiceFactory::GetForBrowserState(browser_state_);
}

void ChromeAutofillClientIOS::OfferPlusAddressCreation(
    const url::Origin& main_frame_origin,
    PlusAddressCallback callback) {
  AutofillBottomSheetTabHelper* bottomSheetTabHelper =
      AutofillBottomSheetTabHelper::FromWebState(web_state_);
  bottomSheetTabHelper->ShowPlusAddressesBottomSheet(std::move(callback));
}

void ChromeAutofillClientIOS::UpdateAutofillPopupDataListValues(
    base::span<const autofill::SelectOption> datalist) {
  // No op. ios/web_view does not support display datalist.
}

std::vector<Suggestion> ChromeAutofillClientIOS::GetPopupSuggestions() const {
  NOTIMPLEMENTED();
  return {};
}

void ChromeAutofillClientIOS::PinPopupView() {
  NOTIMPLEMENTED();
}

void ChromeAutofillClientIOS::UpdatePopup(
    const std::vector<Suggestion>& suggestions,
    FillingProduct main_filling_product,
    AutofillSuggestionTriggerSource trigger_source) {
  NOTIMPLEMENTED();
}

void ChromeAutofillClientIOS::HideAutofillPopup(PopupHidingReason reason) {
  [bridge_ hideAutofillPopup];
}

bool ChromeAutofillClientIOS::IsAutocompleteEnabled() const {
  return prefs::IsAutocompleteEnabled(GetPrefs());
}

bool ChromeAutofillClientIOS::IsPasswordManagerEnabled() {
  return GetPrefs()->GetBoolean(
      password_manager::prefs::kCredentialsEnableService);
}

void ChromeAutofillClientIOS::DidFillOrPreviewForm(
    mojom::ActionPersistence action_persistence,
    AutofillTriggerSource trigger_source,
    bool is_refill) {}

void ChromeAutofillClientIOS::DidFillOrPreviewField(
    const std::u16string& autofilled_value,
    const std::u16string& profile_full_name) {}

bool ChromeAutofillClientIOS::IsContextSecure() const {
  return IsContextSecureForWebState(web_state_);
}

void ChromeAutofillClientIOS::OpenPromoCodeOfferDetailsURL(const GURL& url) {
  web_state_->OpenURL(web::WebState::OpenURLParams(
      url, web::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL,
      /*is_renderer_initiated=*/false));
}

FormInteractionsFlowId
ChromeAutofillClientIOS::GetCurrentFormInteractionsFlowId() {
  // Currently not in use here. See `ChromeAutofillClient` for a proper
  // implementation.
  return {};
}

LogManager* ChromeAutofillClientIOS::GetLogManager() const {
  return log_manager_.get();
}

bool ChromeAutofillClientIOS::IsLastQueriedField(FieldGlobalId field_id) {
  return [bridge_ isLastQueriedField:field_id];
}

bool ChromeAutofillClientIOS::ShouldFormatForLargeKeyboardAccessory() const {
  return IsKeyboardAccessoryUpgradeEnabled();
}

std::unique_ptr<device_reauth::DeviceAuthenticator>
ChromeAutofillClientIOS::GetDeviceAuthenticator() {
  device_reauth::DeviceAuthParams params(
      base::Seconds(60), device_reauth::DeviceAuthSource::kAutofill);
  id<ReauthenticationProtocol> reauthModule =
      ScopedAutofillPaymentReauthModuleOverride::Get();
  if (!reauthModule) {
    reauthModule = [[ReauthenticationModule alloc] init];
  }
  return CreateIOSDeviceAuthenticator(reauthModule, browser_state_, params);
}
VirtualCardEnrollmentManager*
ChromeAutofillClientIOS::GetVirtualCardEnrollmentManager() {
  return form_data_importer_->GetVirtualCardEnrollmentManager();
}

void ChromeAutofillClientIOS::ShowVirtualCardEnrollDialog(
    const VirtualCardEnrollmentFields& virtual_card_enrollment_fields,
    base::OnceClosure accept_virtual_card_callback,
    base::OnceClosure decline_virtual_card_callback) {
  AutofillBottomSheetTabHelper* bottomSheetTabHelper =
      AutofillBottomSheetTabHelper::FromWebState(web_state_);
  bottomSheetTabHelper->ShowVirtualCardEnrollmentBottomSheet(
      VirtualCardEnrollUiModel::Create(virtual_card_enrollment_fields),
      VirtualCardEnrollmentCallbacks(std::move(accept_virtual_card_callback),
                                     std::move(decline_virtual_card_callback)));
}

std::optional<std::u16string> ChromeAutofillClientIOS::GetUserEmail() {
  AuthenticationService* authenticationService =
      AuthenticationServiceFactory::GetForBrowserState(browser_state_);
  DCHECK(authenticationService);
  id<SystemIdentity> identity =
      authenticationService->GetPrimaryIdentity(signin::ConsentLevel::kSignin);
  if (identity) {
    return base::SysNSStringToUTF16(identity.userEmail);
  }
  return std::nullopt;
}

}  // namespace autofill
