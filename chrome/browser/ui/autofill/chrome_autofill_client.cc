// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"

#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/rtl.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/address_normalizer_factory.h"
#include "chrome/browser/autofill/autocomplete_history_manager_factory.h"
#include "chrome/browser/autofill/autofill_offer_manager_factory.h"
#include "chrome/browser/autofill/autofill_optimization_guide_factory.h"
#include "chrome/browser/autofill/iban_manager_factory.h"
#include "chrome/browser/autofill/merchant_promo_code_manager_factory.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/autofill/strike_database_factory.h"
#include "chrome/browser/autofill/ui/ui_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/device_reauth/chrome_device_authenticator_factory.h"
#include "chrome/browser/fast_checkout/fast_checkout_client_impl.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/keyboard_accessory/android/manual_filling_controller.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/password_manager/password_manager_settings_service_factory.h"
#include "chrome/browser/plus_addresses/plus_address_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/signin_promo_util.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/autofill/address_bubbles_controller.h"
#include "chrome/browser/ui/autofill/autofill_field_promo_controller_impl.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/autofill_snackbar_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/chrome_payments_autofill_client.h"
#include "chrome/browser/ui/autofill/payments/credit_card_scanner_controller.h"
#include "chrome/browser/ui/autofill/payments/iban_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/mandatory_reauth_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/view_factory.h"
#include "chrome/browser/ui/autofill/payments/virtual_card_enroll_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/popup_controller_common.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/hats/hats_service.h"
#include "chrome/browser/ui/hats/hats_service_factory.h"
#include "chrome/browser/ui/hats/survey_config.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/browser/ui/passwords/ui_utils.h"
#include "chrome/browser/ui/plus_addresses/plus_address_creation_controller.h"
#include "chrome/browser/webdata_services/web_data_service_factory.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/url_constants.h"
#include "components/autofill/content/browser/autofill_log_router_factory.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_optimization_guide.h"
#include "components/autofill/core/browser/autofill_plus_address_delegate.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/data_model/autofill_offer_data.h"
#include "components/autofill/core/browser/data_model/autofill_profile.h"
#include "components/autofill/core/browser/form_data_importer.h"
#include "components/autofill/core/browser/payments/card_unmask_challenge_option.h"
#include "components/autofill/core/browser/payments/credit_card_cvc_authenticator.h"
#include "components/autofill/core/browser/payments/credit_card_otp_authenticator.h"
#include "components/autofill/core/browser/payments/credit_card_risk_based_authenticator.h"
#include "components/autofill/core/browser/payments/iban_access_manager.h"
#include "components/autofill/core/browser/payments/mandatory_reauth_manager.h"
#include "components/autofill/core/browser/payments/offer_notification_options.h"
#include "components/autofill/core/browser/payments/payments_network_interface.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/ui/payments/bubble_show_options.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_authentication_selection_dialog_controller_impl.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_otp_input_dialog_controller_impl.h"
#include "components/autofill/core/browser/ui/popup_item_ids.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/form_interactions_flow.h"
#include "components/autofill/core/common/unique_ids.h"
#include "components/compose/buildflags.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/optimization_guide/machine_learning_tflite_buildflags.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_setting.h"
#include "components/password_manager/core/browser/password_manager_settings_service.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/browser/password_requirements_service.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/plus_addresses/features.h"
#include "components/plus_addresses/plus_address_types.h"
#include "components/prefs/pref_service.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/security_state/core/security_state.h"
#include "components/signin/public/base/signin_metrics.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/sync/service/sync_service.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/unified_consent/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "components/webauthn/content/browser/internal_authenticator_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/storage_partition.h"
#include "delete_address_profile_dialog_controller_impl.h"
#include "edit_address_profile_dialog_controller_impl.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/gfx/geometry/rect.h"
#include "url/origin.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/android/preferences/autofill/settings_launcher_helper.h"
#include "chrome/browser/android/signin/signin_bridge.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/flags/android/chrome_feature_list.h"
#include "chrome/browser/touch_to_fill/autofill/android/touch_to_fill_payment_method_view_impl.h"
#include "chrome/browser/ui/android/autofill/autofill_accessibility_utils.h"
#include "chrome/browser/ui/android/autofill/autofill_cvc_save_message_delegate.h"
#include "chrome/browser/ui/android/autofill/autofill_logger_android.h"
#include "chrome/browser/ui/android/autofill/autofill_save_card_bottom_sheet_bridge.h"
#include "chrome/browser/ui/android/autofill/autofill_save_card_delegate_android.h"
#include "chrome/browser/ui/android/autofill/card_expiration_date_fix_flow_view_android.h"
#include "chrome/browser/ui/android/autofill/card_name_fix_flow_view_android.h"
#include "chrome/browser/ui/android/infobars/autofill_credit_card_filling_infobar.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/autofill/payments/autofill_snackbar_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/offer_notification_controller_android.h"
#include "components/autofill/core/browser/payments/autofill_credit_card_filling_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/payments/autofill_save_card_infobar_delegate_mobile.h"
#include "components/autofill/core/browser/payments/autofill_save_card_infobar_mobile.h"
#include "components/autofill/core/browser/ui/payments/card_expiration_date_fix_flow_view.h"
#include "components/autofill/core/browser/ui/payments/card_name_fix_flow_view.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/messages/android/messages_feature.h"
#include "components/strings/grit/components_strings.h"
#include "components/webauthn/android/internal_authenticator_android.h"
#include "ui/android/window_android.h"
#else  // BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/autofill/delete_address_profile_dialog_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/offer_notification_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/virtual_card_manual_fallback_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/virtual_card_selection_dialog_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/webauthn_dialog.h"
#include "chrome/browser/ui/autofill/payments/webauthn_dialog_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/webauthn_dialog_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_COMPOSE)
#include "chrome/browser/compose/chrome_compose_client.h"
#include "components/compose/core/browser/compose_manager.h"  // nogncheck
#endif

#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
#include "chrome/browser/autofill/autofill_ml_prediction_model_service_factory.h"
#include "components/autofill/core/browser/ml_model/autofill_ml_prediction_model_handler.h"
#endif

namespace autofill {

namespace {

AutoselectFirstSuggestion ShouldAutofillPopupAutoselectFirstSuggestion(
    AutofillSuggestionTriggerSource source) {
  return AutoselectFirstSuggestion(
      source == AutofillSuggestionTriggerSource::kTextFieldDidReceiveKeyDown);
}

}  // namespace

// static
void ChromeAutofillClient::CreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(
        UserDataKey(),
        base::WrapUnique(new ChromeAutofillClient(web_contents)));
  }
}

ChromeAutofillClient::~ChromeAutofillClient() {
  // NOTE: It is too late to clean up the autofill popup; that cleanup process
  // requires that the WebContents instance still be valid and it is not at
  // this point (in particular, the WebContentsImpl destructor has already
  // finished running and we are now in the base class destructor).
  DCHECK(!popup_controller_);
}

version_info::Channel ChromeAutofillClient::GetChannel() const {
  return chrome::GetChannel();
}

bool ChromeAutofillClient::IsOffTheRecord() const {
  auto* mutable_this = const_cast<ChromeAutofillClient*>(this);
  return mutable_this->web_contents()->GetBrowserContext()->IsOffTheRecord();
}

scoped_refptr<network::SharedURLLoaderFactory>
ChromeAutofillClient::GetURLLoaderFactory() {
  return web_contents()
      ->GetBrowserContext()
      ->GetDefaultStoragePartition()
      ->GetURLLoaderFactoryForBrowserProcess();
}

AutofillCrowdsourcingManager* ChromeAutofillClient::GetCrowdsourcingManager() {
  if (!crowdsourcing_manager_) {
    // Lazy initialization to avoid virtual function calls in the constructor.
    crowdsourcing_manager_ = std::make_unique<AutofillCrowdsourcingManager>(
        this, GetChannel(), GetLogManager());
  }
  return crowdsourcing_manager_.get();
}

AutofillOptimizationGuide* ChromeAutofillClient::GetAutofillOptimizationGuide()
    const {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return profile->ShutdownStarted()
             ? nullptr
             : AutofillOptimizationGuideFactory::GetForProfile(profile);
}

AutofillMlPredictionModelHandler*
ChromeAutofillClient::GetAutofillMlPredictionModelHandler() {
#if BUILDFLAG(BUILD_WITH_TFLITE_LIB)
  if (base::FeatureList::IsEnabled(features::kAutofillModelPredictions)) {
    return AutofillMlPredictionModelServiceFactory::GetForBrowserContext(
        web_contents()->GetBrowserContext());
  }
#endif
  return nullptr;
}

PersonalDataManager* ChromeAutofillClient::GetPersonalDataManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return PersonalDataManagerFactory::GetForProfile(profile);
}

AutocompleteHistoryManager*
ChromeAutofillClient::GetAutocompleteHistoryManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return AutocompleteHistoryManagerFactory::GetForProfile(profile);
}

IbanManager* ChromeAutofillClient::GetIbanManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return IbanManagerFactory::GetForProfile(profile);
}

IbanAccessManager* ChromeAutofillClient::GetIbanAccessManager() {
  if (!iban_access_manager_) {
    iban_access_manager_ = std::make_unique<IbanAccessManager>(this);
  }
  return iban_access_manager_.get();
}

AutofillComposeDelegate* ChromeAutofillClient::GetComposeDelegate() {
#if BUILDFLAG(ENABLE_COMPOSE)
  auto* client = ChromeComposeClient::FromWebContents(web_contents());
  return client ? &client->GetManager() : nullptr;
#else
  return nullptr;
#endif
}

AutofillPlusAddressDelegate* ChromeAutofillClient::GetPlusAddressDelegate() {
  // The `PlusAddressServiceFactory` should also ensure the service is not
  // created without the feature enabled, but being defensive here to avoid
  // surprises.
  if (!base::FeatureList::IsEnabled(
          plus_addresses::features::kPlusAddressesEnabled)) {
    return nullptr;
  }
  return PlusAddressServiceFactory::GetForBrowserContext(
      web_contents()->GetBrowserContext());
}

void ChromeAutofillClient::OfferPlusAddressCreation(
    const url::Origin& main_frame_origin,
    PlusAddressCallback callback) {
  // The controller is owned by `web_contents()` (via `WebContentsUserData`).
  plus_addresses::PlusAddressCreationController* controller =
      plus_addresses::PlusAddressCreationController::GetOrCreate(
          web_contents());
  controller->OfferCreation(main_frame_origin, std::move(callback));
}

MerchantPromoCodeManager* ChromeAutofillClient::GetMerchantPromoCodeManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return MerchantPromoCodeManagerFactory::GetForProfile(profile);
}

CreditCardCvcAuthenticator* ChromeAutofillClient::GetCvcAuthenticator() {
  if (!cvc_authenticator_)
    cvc_authenticator_ = std::make_unique<CreditCardCvcAuthenticator>(this);
  return cvc_authenticator_.get();
}

CreditCardOtpAuthenticator* ChromeAutofillClient::GetOtpAuthenticator() {
  if (!otp_authenticator_)
    otp_authenticator_ = std::make_unique<CreditCardOtpAuthenticator>(this);
  return otp_authenticator_.get();
}

CreditCardRiskBasedAuthenticator*
ChromeAutofillClient::GetRiskBasedAuthenticator() {
  if (!risk_based_authenticator_) {
    risk_based_authenticator_ =
        std::make_unique<CreditCardRiskBasedAuthenticator>(this);
  }
  return risk_based_authenticator_.get();
}

PrefService* ChromeAutofillClient::GetPrefs() {
  return const_cast<PrefService*>(std::as_const(*this).GetPrefs());
}

const PrefService* ChromeAutofillClient::GetPrefs() const {
  return Profile::FromBrowserContext(web_contents()->GetBrowserContext())
      ->GetPrefs();
}

syncer::SyncService* ChromeAutofillClient::GetSyncService() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return SyncServiceFactory::GetForProfile(profile);
}

signin::IdentityManager* ChromeAutofillClient::GetIdentityManager() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return IdentityManagerFactory::GetForProfile(profile->GetOriginalProfile());
}

FormDataImporter* ChromeAutofillClient::GetFormDataImporter() {
  if (!form_data_importer_) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents()->GetBrowserContext());
    form_data_importer_ = std::make_unique<FormDataImporter>(
        this, GetPersonalDataManager(),
        HistoryServiceFactory::GetForProfile(
            profile, ServiceAccessType::EXPLICIT_ACCESS),
        GetPersonalDataManager()->app_locale());
  }
  return form_data_importer_.get();
}

payments::ChromePaymentsAutofillClient*
ChromeAutofillClient::GetPaymentsAutofillClient() {
  if (!payments_autofill_client_) {
    payments_autofill_client_ =
        std::make_unique<payments::ChromePaymentsAutofillClient>(this);
  }
  return payments_autofill_client_.get();
}

StrikeDatabase* ChromeAutofillClient::GetStrikeDatabase() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  // No need to return a StrikeDatabase in incognito mode. It is primarily
  // used to determine whether or not to offer save of Autofill data. However,
  // we don't allow saving of Autofill data while in incognito anyway, so an
  // incognito code path should never get far enough to query StrikeDatabase.
  return StrikeDatabaseFactory::GetForProfile(profile);
}

ukm::UkmRecorder* ChromeAutofillClient::GetUkmRecorder() {
  return ukm::UkmRecorder::Get();
}

ukm::SourceId ChromeAutofillClient::GetUkmSourceId() {
  return web_contents()->GetPrimaryMainFrame()->GetPageUkmSourceId();
}

AddressNormalizer* ChromeAutofillClient::GetAddressNormalizer() {
  return AddressNormalizerFactory::GetInstance();
}

AutofillOfferManager* ChromeAutofillClient::GetAutofillOfferManager() {
  return AutofillOfferManagerFactory::GetForBrowserContext(
      web_contents()->GetBrowserContext());
}

const GURL& ChromeAutofillClient::GetLastCommittedPrimaryMainFrameURL() const {
  return web_contents()->GetPrimaryMainFrame()->GetLastCommittedURL();
}

url::Origin ChromeAutofillClient::GetLastCommittedPrimaryMainFrameOrigin()
    const {
  return web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
}

security_state::SecurityLevel
ChromeAutofillClient::GetSecurityLevelForUmaHistograms() {
  SecurityStateTabHelper* helper =
      ::SecurityStateTabHelper::FromWebContents(web_contents());

  // If there is no helper, it means we are not in a "web" state (for example
  // the file picker on CrOS). Return SECURITY_LEVEL_COUNT which will not be
  // logged.
  if (!helper)
    return security_state::SecurityLevel::SECURITY_LEVEL_COUNT;

  return helper->GetSecurityLevel();
}

const translate::LanguageState* ChromeAutofillClient::GetLanguageState() {
  // TODO(crbug.com/912597): iOS vs other platforms extracts the language from
  // the top level frame vs whatever frame directly holds the form.
  auto* translate_manager =
      ChromeTranslateClient::GetManagerFromWebContents(web_contents());
  if (translate_manager)
    return translate_manager->GetLanguageState();
  return nullptr;
}

translate::TranslateDriver* ChromeAutofillClient::GetTranslateDriver() {
  // TODO(crbug.com/912597): iOS vs other platforms extracts the language from
  // the top level frame vs whatever frame directly holds the form.
  auto* translate_client =
      ChromeTranslateClient::FromWebContents(web_contents());
  if (translate_client)
    return translate_client->translate_driver();
  return nullptr;
}

GeoIpCountryCode ChromeAutofillClient::GetVariationConfigCountryCode() const {
  variations::VariationsService* variation_service =
      g_browser_process->variations_service();
  // Retrieves the country code from variation service and converts it to upper
  // case.
  return GeoIpCountryCode(
      variation_service
          ? base::ToUpperASCII(variation_service->GetLatestCountry())
          : std::string());
}

profile_metrics::BrowserProfileType ChromeAutofillClient::GetProfileType()
    const {
  Profile* profile = GetProfile();
  // Profile can only be null in tests, therefore it is safe to always return
  // |kRegular| when it does not exist.
  return profile ? profile_metrics::GetBrowserProfileType(profile)
                 : profile_metrics::BrowserProfileType::kRegular;
}

FastCheckoutClient* ChromeAutofillClient::GetFastCheckoutClient() {
#if BUILDFLAG(IS_ANDROID)
  return fast_checkout_client_.get();
#else
  return nullptr;
#endif
}

std::unique_ptr<webauthn::InternalAuthenticator>
ChromeAutofillClient::CreateCreditCardInternalAuthenticator(
    AutofillDriver* driver) {
  auto* cad = static_cast<ContentAutofillDriver*>(driver);
  content::RenderFrameHost* rfh = cad->render_frame_host();
#if BUILDFLAG(IS_ANDROID)
  return std::make_unique<webauthn::InternalAuthenticatorAndroid>(rfh);
#else
  return std::make_unique<content::InternalAuthenticatorImpl>(rfh);
#endif
}

void ChromeAutofillClient::ShowAutofillSettings(
    FillingProduct main_filling_product) {
  DCHECK(main_filling_product != FillingProduct::kPassword);
#if BUILDFLAG(IS_ANDROID)
  switch (main_filling_product) {
    case FillingProduct::kAddress:
      ShowAutofillProfileSettings(web_contents());
      return;
    case FillingProduct::kCreditCard:
      ShowAutofillCreditCardSettings(web_contents());
      return;
    case FillingProduct::kAutocomplete:
    case FillingProduct::kCompose:
    case FillingProduct::kIban:
    case FillingProduct::kMerchantPromoCode:
    case FillingProduct::kPassword:
    case FillingProduct::kPlusAddresses:
    case FillingProduct::kNone:
      NOTREACHED();
  }
#else
  Browser* browser = chrome::FindBrowserWithTab(web_contents());
  if (browser) {
    switch (main_filling_product) {
      case FillingProduct::kAddress:
        chrome::ShowSettingsSubPage(browser, chrome::kAddressesSubPage);
        return;
      case FillingProduct::kCreditCard:
      case FillingProduct::kIban:
        chrome::ShowSettingsSubPage(browser, chrome::kPaymentsSubPage);
        return;
      case FillingProduct::kAutocomplete:
      case FillingProduct::kCompose:
      case FillingProduct::kMerchantPromoCode:
      case FillingProduct::kPassword:
      case FillingProduct::kPlusAddresses:
      case FillingProduct::kNone:
        NOTREACHED();
    }
  }
#endif  // BUILDFLAG(IS_ANDROID)
}

void ChromeAutofillClient::ShowUnmaskAuthenticatorSelectionDialog(
    const std::vector<CardUnmaskChallengeOption>& challenge_options,
    base::OnceCallback<void(const std::string&)>
        confirm_unmask_challenge_option_callback,
    base::OnceClosure cancel_unmasking_closure) {
  CHECK(!card_unmask_authentication_selection_controller_);
  card_unmask_authentication_selection_controller_ =
      std::make_unique<CardUnmaskAuthenticationSelectionDialogControllerImpl>(
          challenge_options,
          std::move(confirm_unmask_challenge_option_callback),
          std::move(cancel_unmasking_closure));
  card_unmask_authentication_selection_controller_->ShowDialog(
      base::BindOnce(&CreateAndShowCardUnmaskAuthenticationSelectionDialog,
                     &GetWebContents()));
}

void ChromeAutofillClient::DismissUnmaskAuthenticatorSelectionDialog(
    bool server_success) {
  if (card_unmask_authentication_selection_controller_) {
    card_unmask_authentication_selection_controller_
        ->DismissDialogUponServerProcessedAuthenticationMethodRequest(
            server_success);
    card_unmask_authentication_selection_controller_.reset();
  }
}

VirtualCardEnrollmentManager*
ChromeAutofillClient::GetVirtualCardEnrollmentManager() {
  return form_data_importer_->GetVirtualCardEnrollmentManager();
}

void ChromeAutofillClient::ShowVirtualCardEnrollDialog(
    const VirtualCardEnrollmentFields& virtual_card_enrollment_fields,
    base::OnceClosure accept_virtual_card_callback,
    base::OnceClosure decline_virtual_card_callback) {
  VirtualCardEnrollBubbleControllerImpl::CreateForWebContents(web_contents());
  VirtualCardEnrollBubbleControllerImpl* controller =
      VirtualCardEnrollBubbleControllerImpl::FromWebContents(web_contents());
  DCHECK(controller);
  controller->ShowBubble(virtual_card_enrollment_fields,
                         std::move(accept_virtual_card_callback),
                         std::move(decline_virtual_card_callback));
}

payments::MandatoryReauthManager*
ChromeAutofillClient::GetOrCreatePaymentsMandatoryReauthManager() {
  if (!payments_mandatory_reauth_manager_) {
    payments_mandatory_reauth_manager_ =
        std::make_unique<payments::MandatoryReauthManager>(this);
  }

  return payments_mandatory_reauth_manager_.get();
}

void ChromeAutofillClient::ShowMandatoryReauthOptInPrompt(
    base::OnceClosure accept_mandatory_reauth_callback,
    base::OnceClosure cancel_mandatory_reauth_callback,
    base::RepeatingClosure close_mandatory_reauth_callback) {
  MandatoryReauthBubbleControllerImpl::CreateForWebContents(web_contents());
  MandatoryReauthBubbleControllerImpl::FromWebContents(web_contents())
      ->ShowBubble(std::move(accept_mandatory_reauth_callback),
                   std::move(cancel_mandatory_reauth_callback),
                   std::move(close_mandatory_reauth_callback));
}

void ChromeAutofillClient::ShowMandatoryReauthOptInConfirmation() {
#if BUILDFLAG(IS_ANDROID)
  if (!autofill_snackbar_controller_impl_) {
    autofill_snackbar_controller_impl_ =
        std::make_unique<AutofillSnackbarControllerImpl>(web_contents());
  }
  autofill_snackbar_controller_impl_->Show(
      AutofillSnackbarType::kMandatoryReauth);
#else
  MandatoryReauthBubbleControllerImpl::CreateForWebContents(web_contents());
  // TODO(crbug.com/4555994): Pass in the bubble type as a parameter so we
  // enforce that the confirmation bubble is shown.
  MandatoryReauthBubbleControllerImpl::FromWebContents(web_contents())
      ->ReshowBubble();
#endif
}

#if !BUILDFLAG(IS_ANDROID)
void ChromeAutofillClient::HideVirtualCardEnrollBubbleAndIconIfVisible() {
  VirtualCardEnrollBubbleControllerImpl::CreateForWebContents(web_contents());
  VirtualCardEnrollBubbleControllerImpl* controller =
      VirtualCardEnrollBubbleControllerImpl::FromWebContents(web_contents());

  if (controller && controller->IsIconVisible())
    controller->HideIconAndBubble();
}
#endif

#if !BUILDFLAG(IS_ANDROID)
void ChromeAutofillClient::ShowWebauthnOfferDialog(
    WebauthnDialogCallback offer_dialog_callback) {
  WebauthnDialogControllerImpl::GetOrCreateForPage(
      web_contents()->GetPrimaryPage())
      ->ShowOfferDialog(std::move(offer_dialog_callback));
}

void ChromeAutofillClient::ShowWebauthnVerifyPendingDialog(
    WebauthnDialogCallback verify_pending_dialog_callback) {
  WebauthnDialogControllerImpl::GetOrCreateForPage(
      web_contents()->GetPrimaryPage())
      ->ShowVerifyPendingDialog(std::move(verify_pending_dialog_callback));
}

void ChromeAutofillClient::UpdateWebauthnOfferDialogWithError() {
  WebauthnDialogControllerImpl* controller =
      WebauthnDialogControllerImpl::GetForPage(
          web_contents()->GetPrimaryPage());
  if (controller)
    controller->UpdateDialog(WebauthnDialogState::kOfferError);
}

bool ChromeAutofillClient::CloseWebauthnDialog() {
  WebauthnDialogControllerImpl* controller =
      WebauthnDialogControllerImpl::GetForPage(
          web_contents()->GetPrimaryPage());
  if (controller)
    return controller->CloseDialog();

  return false;
}

void ChromeAutofillClient::OfferVirtualCardOptions(
    const std::vector<raw_ptr<CreditCard, VectorExperimental>>& candidates,
    base::OnceCallback<void(const std::string&)> callback) {
  VirtualCardSelectionDialogControllerImpl::CreateForWebContents(
      web_contents());
  VirtualCardSelectionDialogControllerImpl::FromWebContents(web_contents())
      ->ShowDialog(candidates, std::move(callback));
}

#else  // BUILDFLAG(IS_ANDROID)
void ChromeAutofillClient::ConfirmAccountNameFixFlow(
    base::OnceCallback<void(const std::u16string&)> callback) {
  CardNameFixFlowViewAndroid* card_name_fix_flow_view_android =
      new CardNameFixFlowViewAndroid(&card_name_fix_flow_controller_,
                                     web_contents());
  card_name_fix_flow_controller_.Show(
      card_name_fix_flow_view_android, GetAccountHolderName(),
      /*upload_save_card_callback=*/std::move(callback));
}

void ChromeAutofillClient::ConfirmExpirationDateFixFlow(
    const CreditCard& card,
    base::OnceCallback<void(const std::u16string&, const std::u16string&)>
        callback) {
  CardExpirationDateFixFlowViewAndroid*
      card_expiration_date_fix_flow_view_android =
          new CardExpirationDateFixFlowViewAndroid(
              &card_expiration_date_fix_flow_controller_, web_contents());
  card_expiration_date_fix_flow_controller_.Show(
      card_expiration_date_fix_flow_view_android, card,
      /*upload_save_card_callback=*/std::move(callback));
}
#endif

void ChromeAutofillClient::ConfirmSaveCreditCardLocally(
    const CreditCard& card,
    SaveCreditCardOptions options,
    LocalSaveCardPromptCallback callback) {
#if BUILDFLAG(IS_ANDROID)
  DCHECK(options.show_prompt);
  AutofillSaveCardUiInfo ui_info =
      AutofillSaveCardUiInfo::CreateForLocalSave(options, card);
  auto save_card_delegate = std::make_unique<AutofillSaveCardDelegateAndroid>(
      std::move(callback), options, web_contents());

  // If a CVC is detected for an existing local card in the checkout form, the
  // CVC save prompt is shown in a message.
  if (options.card_save_type ==
      ChromeAutofillClient::CardSaveType::kCvcSaveOnly) {
    autofill_cvc_save_message_delegate_ =
        std::make_unique<AutofillCvcSaveMessageDelegate>(web_contents());
    autofill_cvc_save_message_delegate_->ShowMessage(
        ui_info, std::move(save_card_delegate));
    return;
  }

  // Saving a new local card (may include CVC) via a bottom sheet.
  if (auto* bridge = GetOrCreateAutofillSaveCardBottomSheetBridge()) {
    bridge->RequestShowContent(ui_info, std::move(save_card_delegate));
  }
#else
  // Do lazy initialization of SaveCardBubbleControllerImpl.
  SaveCardBubbleControllerImpl::CreateForWebContents(web_contents());
  SaveCardBubbleControllerImpl::FromWebContents(web_contents())
      ->OfferLocalSave(card, options, std::move(callback));
#endif
}

void ChromeAutofillClient::ConfirmSaveCreditCardToCloud(
    const CreditCard& card,
    const LegalMessageLines& legal_message_lines,
    SaveCreditCardOptions options,
    UploadSaveCardPromptCallback callback) {
#if BUILDFLAG(IS_ANDROID)
  DCHECK(options.show_prompt);
  signin::IdentityManager* identity_manager =
      IdentityManagerFactory::GetForProfile(GetProfile());
  AccountInfo account_info = identity_manager->FindExtendedAccountInfo(
      identity_manager->GetPrimaryAccountInfo(signin::ConsentLevel::kSignin));
  AutofillSaveCardUiInfo ui_info = AutofillSaveCardUiInfo::CreateForUploadSave(
      options, card, legal_message_lines, account_info);
  auto save_card_delegate = std::make_unique<AutofillSaveCardDelegateAndroid>(
      std::move(callback), options, web_contents());

  // If a CVC is detected for an existing server card in the checkout form, the
  // CVC save prompt is shown in a message.
  if (options.card_save_type == AutofillClient::CardSaveType::kCvcSaveOnly) {
      autofill_cvc_save_message_delegate_ =
          std::make_unique<AutofillCvcSaveMessageDelegate>(web_contents());
      autofill_cvc_save_message_delegate_->ShowMessage(
          ui_info, std::move(save_card_delegate));
      return;
  }

  // For new cards, the save card prompt is shown in a bottom sheet.
  if (auto* bridge = GetOrCreateAutofillSaveCardBottomSheetBridge()) {
    bridge->RequestShowContent(ui_info, std::move(save_card_delegate));
  }
#else
  // Hide virtual card confirmation bubble showing for a different card.
  HideVirtualCardEnrollBubbleAndIconIfVisible();

  // Do lazy initialization of SaveCardBubbleControllerImpl.
  SaveCardBubbleControllerImpl::CreateForWebContents(web_contents());
  SaveCardBubbleControllerImpl::FromWebContents(web_contents())
      ->OfferUploadSave(card, legal_message_lines, options,
                        std::move(callback));
#endif
}

void ChromeAutofillClient::ConfirmSaveIbanLocally(
    const Iban& iban,
    bool should_show_prompt,
    SaveIbanPromptCallback callback) {
#if !BUILDFLAG(IS_ANDROID)
  // Do lazy initialization of IbanBubbleControllerImpl.
  IbanBubbleControllerImpl::CreateForWebContents(web_contents());
  IbanBubbleControllerImpl::FromWebContents(web_contents())
      ->OfferLocalSave(iban, should_show_prompt, std::move(callback));
#endif
}

void ChromeAutofillClient::ConfirmUploadIbanToCloud(
    const Iban& iban,
    LegalMessageLines legal_message_lines,
    bool should_show_prompt,
    SaveIbanPromptCallback callback) {
#if !BUILDFLAG(IS_ANDROID)
  // Do lazy initialization of IbanBubbleControllerImpl.
  IbanBubbleControllerImpl::CreateForWebContents(web_contents());
  IbanBubbleControllerImpl::FromWebContents(web_contents())
      ->OfferUploadSave(iban, std::move(legal_message_lines),
                        should_show_prompt, std::move(callback));
#endif
}

void ChromeAutofillClient::ConfirmCreditCardFillAssist(
    const CreditCard& card,
    base::OnceClosure callback) {
#if BUILDFLAG(IS_ANDROID)
  auto infobar_delegate =
      std::make_unique<AutofillCreditCardFillingInfoBarDelegateMobile>(
          card, std::move(callback));
  auto* raw_delegate = infobar_delegate.get();
  if (infobars::ContentInfoBarManager::FromWebContents(web_contents())
          ->AddInfoBar(std::make_unique<AutofillCreditCardFillingInfoBar>(
              std::move(infobar_delegate)))) {
    raw_delegate->set_was_shown();
  }
#endif
}

void ChromeAutofillClient::ShowEditAddressProfileDialog(
    const AutofillProfile& profile,
    AddressProfileSavePromptCallback on_user_decision_callback) {
#if !BUILDFLAG(IS_ANDROID)
  EditAddressProfileDialogControllerImpl::CreateForWebContents(web_contents());
  EditAddressProfileDialogControllerImpl* controller =
      EditAddressProfileDialogControllerImpl::FromWebContents(web_contents());
  CHECK(controller);

  std::optional<AccountInfo> account = GetPrimaryAccountInfoFromBrowserContext(
      web_contents()->GetBrowserContext());
  CHECK(account);
  std::u16string footer_message =
      profile.source() == AutofillProfile::Source::kAccount
          ? l10n_util::GetStringFUTF16(
                IDS_AUTOFILL_UPDATE_PROMPT_ACCOUNT_ADDRESS_SOURCE_NOTICE,
                base::ASCIIToUTF16(account->email))
          : u"";
  controller->OfferEdit(
      /*profile=*/profile,
      /*title_override=*/u"", footer_message,
      /*is_editing_existing_address=*/false,
      /*is_migration_to_account=*/false,
      /*on_user_decision_callback=*/std::move(on_user_decision_callback));
#else
  // Edit address profile dialog is only available is desktop.
  NOTREACHED_NORETURN();
#endif
}

void ChromeAutofillClient::ShowDeleteAddressProfileDialog(
    const AutofillProfile& profile,
    AddressProfileDeleteDialogCallback delete_dialog_callback) {
#if !BUILDFLAG(IS_ANDROID)
  DeleteAddressProfileDialogControllerImpl::CreateForWebContents(
      web_contents());
  DeleteAddressProfileDialogControllerImpl* controller =
      DeleteAddressProfileDialogControllerImpl::FromWebContents(web_contents());
  controller->OfferDelete(
      /*is_account_address_profile=*/profile.source() ==
          AutofillProfile::Source::kAccount,
      /*delete_dialog_callback=*/std::move(delete_dialog_callback));
#else
  // Delete address profile dialog is only available is desktop.
  NOTREACHED_NORETURN();
#endif
}

void ChromeAutofillClient::ConfirmSaveAddressProfile(
    const AutofillProfile& profile,
    const AutofillProfile* original_profile,
    SaveAddressProfilePromptOptions options,
    AddressProfileSavePromptCallback callback) {
#if BUILDFLAG(IS_ANDROID)
  // TODO(crbug.com/40164488): Respect SaveAddressProfilePromptOptions.
  save_update_address_profile_flow_manager_.OfferSave(
      web_contents(), profile, original_profile,
      options.is_migration_to_account, std::move(callback));
#else
  AddressBubblesController::SetUpAndShowSaveOrUpdateAddressBubble(
      web_contents(), profile, original_profile, options, std::move(callback));
#endif
}

bool ChromeAutofillClient::HasCreditCardScanFeature() const {
  return CreditCardScannerController::HasCreditCardScanFeature();
}

void ChromeAutofillClient::ScanCreditCard(CreditCardScanCallback callback) {
  CreditCardScannerController::ScanCreditCard(web_contents(),
                                              std::move(callback));
}

bool ChromeAutofillClient::ShowTouchToFillCreditCard(
    base::WeakPtr<TouchToFillDelegate> delegate,
    base::span<const autofill::CreditCard> cards_to_suggest) {
#if BUILDFLAG(IS_ANDROID)
  // Create the manual filling controller which will be used to show the
  // unmasked virtual card details in the manual fallback.
  ManualFillingController::GetOrCreate(web_contents())
      ->UpdateSourceAvailability(
          ManualFillingController::FillingSource::CREDIT_CARD_FALLBACKS,
          !cards_to_suggest.empty());

  return touch_to_fill_payment_method_controller_.Show(
      std::make_unique<TouchToFillPaymentMethodViewImpl>(web_contents()),
      delegate, std::move(cards_to_suggest));
#else
  // Touch To Fill is not supported on Desktop.
  NOTREACHED();
  return false;
#endif
}

void ChromeAutofillClient::HideTouchToFillCreditCard() {
#if BUILDFLAG(IS_ANDROID)
  touch_to_fill_payment_method_controller_.Hide();
#else
  // Touch To Fill is not supported on Desktop.
  NOTREACHED();
#endif
}

void ChromeAutofillClient::ShowAutofillPopup(
    const PopupOpenArgs& open_args,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  // The Autofill Popup cannot open if it overlaps with another popup.
  // Therefore, the IPH is hidden before showing the Autofill Popup.
  HideAutofillFieldIphForManualFallbackFeature();

  // IPH hiding is asynchronous. Posting showing the Autofill Popup
  // guarantees the IPH will be hidden by the time the Autofill Popup will
  // attempt to open. This works because the tasks of hiding the IPH and showing
  // the Autofill Popup are posted on the same thread (UI thread).
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ChromeAutofillClient::ShowAutofillPopupImpl,
                                GetWeakPtr(), open_args, delegate));
}

void ChromeAutofillClient::UpdateAutofillPopupDataListValues(
    base::span<const SelectOption> options) {
  if (popup_controller_.get())
    popup_controller_->UpdateDataListValues(options);
}

std::vector<Suggestion> ChromeAutofillClient::GetPopupSuggestions() const {
  return popup_controller_ ? popup_controller_->GetSuggestions()
                           : std::vector<Suggestion>();
}

void ChromeAutofillClient::PinPopupView() {
  if (popup_controller_.get())
    popup_controller_->PinView();
}

std::optional<AutofillClient::PopupScreenLocation>
ChromeAutofillClient::GetPopupScreenLocation() const {
  return popup_controller_
             ? popup_controller_->GetPopupScreenLocation()
             : std::make_optional<AutofillClient::PopupScreenLocation>();
}

void ChromeAutofillClient::UpdatePopup(
    const std::vector<Suggestion>& suggestions,
    FillingProduct main_filling_product,
    AutofillSuggestionTriggerSource trigger_source) {
  if (!popup_controller_.get())
    return;  // Update only if there is a popup.

  // When a form changes dynamically, |popup_controller_| may hold a delegate of
  // the wrong type, so updating the popup would call into the wrong delegate.
  // Hence, just close the existing popup (crbug/1113241). The cast is needed to
  // access AutofillPopupController::GetMainFillingProduct().
  if (main_filling_product !=
      static_cast<const AutofillPopupController*>(popup_controller_.get())
          ->GetMainFillingProduct()) {
    popup_controller_->Hide(PopupHidingReason::kStaleData);
    return;
  }

  // Calling show will reuse the existing view automatically.
  popup_controller_->Show(
      suggestions, trigger_source,
      ShouldAutofillPopupAutoselectFirstSuggestion(trigger_source));
}

void ChromeAutofillClient::HideAutofillPopup(PopupHidingReason reason) {
  if (popup_controller_.get()) {
    popup_controller_->Hide(reason);
  }
}

void ChromeAutofillClient::UpdateOfferNotification(
    const AutofillOfferData* offer,
    const OfferNotificationOptions& options) {
  DCHECK(offer);
  CreditCard* card =
      offer->GetEligibleInstrumentIds().empty()
          ? nullptr
          : GetPersonalDataManager()->GetCreditCardByInstrumentId(
                offer->GetEligibleInstrumentIds()[0]);

  if (offer->IsCardLinkedOffer() && !card)
    return;

#if BUILDFLAG(IS_ANDROID)
  if (options.notification_has_been_shown) {
    // For Android, if notification has been shown on this merchant, don't show
    // it again.
    return;
  }
  OfferNotificationControllerAndroid::CreateForWebContents(web_contents());
  OfferNotificationControllerAndroid* controller =
      OfferNotificationControllerAndroid::FromWebContents(web_contents());
  controller->ShowIfNecessary(offer, card);
#else
  OfferNotificationBubbleControllerImpl::CreateForWebContents(web_contents());
  OfferNotificationBubbleControllerImpl* controller =
      OfferNotificationBubbleControllerImpl::FromWebContents(web_contents());
  controller->ShowOfferNotificationIfApplicable(offer, card, options);
#endif
}

void ChromeAutofillClient::DismissOfferNotification() {
#if BUILDFLAG(IS_ANDROID)
  OfferNotificationControllerAndroid::CreateForWebContents(web_contents());
  OfferNotificationControllerAndroid* controller =
      OfferNotificationControllerAndroid::FromWebContents(web_contents());
  controller->Dismiss();
#else
  OfferNotificationBubbleControllerImpl* controller =
      OfferNotificationBubbleControllerImpl::FromWebContents(web_contents());
  if (!controller)
    return;

  controller->DismissNotification();
#endif
}

void ChromeAutofillClient::OnVirtualCardDataAvailable(
    const VirtualCardManualFallbackBubbleOptions& options) {
#if BUILDFLAG(IS_ANDROID)
  if (!autofill_snackbar_controller_impl_) {
    autofill_snackbar_controller_impl_ =
        std::make_unique<AutofillSnackbarControllerImpl>(web_contents());
  }
  autofill_snackbar_controller_impl_->Show(AutofillSnackbarType::kVirtualCard);
#else
  VirtualCardManualFallbackBubbleControllerImpl::CreateForWebContents(
      web_contents());
  VirtualCardManualFallbackBubbleControllerImpl* controller =
      VirtualCardManualFallbackBubbleControllerImpl::FromWebContents(
          web_contents());
  controller->ShowBubble(options);
#endif
}

void ChromeAutofillClient::TriggerUserPerceptionOfAutofillSurvey(
    const std::map<std::string, std::string>& field_filling_stats_data) {
#if !BUILDFLAG(IS_ANDROID)
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  HatsService* hats_service =
      HatsServiceFactory::GetForProfile(profile, /*create_if_necessary=*/true);
  CHECK(hats_service);
  // Also add information about whether the granular filling feature is
  // available". The goal is to correlate the user's perception of autofill with
  // the feature.
  hats_service->LaunchDelayedSurveyForWebContents(
      kHatsSurveyTriggerAutofillAddressUserPerception, web_contents(),
      /*timeout_ms=*/5000, /*product_specific_bits_data=*/
      {{"granular filling available",
        base::FeatureList::IsEnabled(
            features::kAutofillGranularFillingAvailable)}},
      field_filling_stats_data);
#endif
}

bool ChromeAutofillClient::IsAutocompleteEnabled() const {
  return prefs::IsAutocompleteEnabled(GetPrefs());
}

bool ChromeAutofillClient::IsPasswordManagerEnabled() {
  PasswordManagerSettingsService* settings_service =
      PasswordManagerSettingsServiceFactory::GetForProfile(GetProfile());
  return settings_service->IsSettingEnabled(
      password_manager::PasswordManagerSetting::kOfferToSavePasswords);
}

void ChromeAutofillClient::DidFillOrPreviewForm(
    mojom::ActionPersistence action_persistence,
    AutofillTriggerSource trigger_source,
    bool is_refill) {
#if BUILDFLAG(IS_ANDROID)
  if (action_persistence == mojom::ActionPersistence::kFill &&
      trigger_source == AutofillTriggerSource::kTouchToFillCreditCard &&
      !is_refill) {
    // TODO(crbug.com/40900538): Test that the message was announced.
    autofill::AnnounceTextForA11y(
        l10n_util::GetStringUTF16(IDS_AUTOFILL_A11Y_ANNOUNCE_FILLED_FORM));
  }
#endif  // BUILDFLAG(IS_ANDROID)
}

void ChromeAutofillClient::DidFillOrPreviewField(
    const std::u16string& autofilled_value,
    const std::u16string& profile_full_name) {
#if BUILDFLAG(IS_ANDROID)
  AutofillLoggerAndroid::DidFillOrPreviewField(autofilled_value,
                                               profile_full_name);
#endif  // BUILDFLAG(IS_ANDROID)
}

bool ChromeAutofillClient::IsContextSecure() const {
  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents());
  if (!helper)
    return false;

  const auto security_level = helper->GetSecurityLevel();
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();

  // Only dangerous security states should prevent autofill.
  //
  // TODO(crbug.com/41307071): Once passive mixed content and legacy TLS are
  // less common, just use IsSslCertificateValid().
  return entry && entry->GetURL().SchemeIsCryptographic() &&
         security_level != security_state::DANGEROUS;
}

void ChromeAutofillClient::OpenPromoCodeOfferDetailsURL(const GURL& url) {
  web_contents()->OpenURL(
      content::OpenURLParams(url, content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL,
                             /*is_renderer_initiated=*/false),
      /*navigation_handle_callback=*/{});
}

LogManager* ChromeAutofillClient::GetLogManager() const {
  return log_manager_.get();
}

FormInteractionsFlowId
ChromeAutofillClient::GetCurrentFormInteractionsFlowId() {
  constexpr base::TimeDelta max_flow_time = base::Minutes(20);
  base::Time now = AutofillClock::Now();

  if (now - flow_id_date_ > max_flow_time || now < flow_id_date_) {
    flow_id_ = FormInteractionsFlowId();
    flow_id_date_ = now;
  }
  return flow_id_;
}

std::unique_ptr<device_reauth::DeviceAuthenticator>
ChromeAutofillClient::GetDeviceAuthenticator() {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  device_reauth::DeviceAuthParams params(
      base::Seconds(60), device_reauth::DeviceAuthSource::kAutofill);

  return ChromeDeviceAuthenticatorFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()), params);
#else
  return nullptr;
#endif
}

void ChromeAutofillClient::ShowAutofillFieldIphForManualFallbackFeature(
    const FormFieldData& field) {
#if !BUILDFLAG(IS_ANDROID)
  if (!autofill_field_promo_controller_manual_fallback_) {
    autofill_field_promo_controller_manual_fallback_ =
        std::make_unique<AutofillFieldPromoControllerImpl>(
            web_contents(),
            feature_engagement::kIPHAutofillManualFallbackFeature,
            kAutofillManualFallbackElementId);
  }
  autofill_field_promo_controller_manual_fallback_->Show(field.bounds);
#endif  // !BUILDFLAG(IS_ANDROID)
}

void ChromeAutofillClient::HideAutofillFieldIphForManualFallbackFeature() {
  if (autofill_field_promo_controller_manual_fallback_) {
    autofill_field_promo_controller_manual_fallback_->Hide();
  }
}

void ChromeAutofillClient::NotifyAutofillManualFallbackUsed() {
#if !BUILDFLAG(IS_ANDROID)
  // Based on the feature config, the IPH will not be shown ever again once the
  // user has used the manual fallback feature. If the user is aware that the
  // manual fallback feature exists, then they shouldn't be spammed with IPHs.
  // The IPH code cannot know if the feature was used or not unless explicitly
  // notified.
  feature_engagement::TrackerFactory::GetForBrowserContext(
      web_contents()->GetBrowserContext())
      ->NotifyUsedEvent(feature_engagement::kIPHAutofillManualFallbackFeature);
#endif  // !BUILDFLAG(IS_ANDROID)
}

ChromeAutofillClient::ChromeAutofillClient(content::WebContents* web_contents)
    : ContentAutofillClient(web_contents),
      content::WebContentsObserver(web_contents),
      log_manager_(
          // TODO(crbug.com/40612524): Replace the closure with a callback to
          // the renderer that indicates if log messages should be sent from the
          // renderer.
          LogManager::Create(AutofillLogRouterFactory::GetForBrowserContext(
                                 web_contents->GetBrowserContext()),
                             base::NullCallback())) {
  // Initialize StrikeDatabase so its cache will be loaded and ready to use
  // when requested by other Autofill classes.
  GetStrikeDatabase();

#if BUILDFLAG(IS_ANDROID)
  fast_checkout_client_ = std::make_unique<FastCheckoutClientImpl>(this);
#endif
}

#if BUILDFLAG(IS_ANDROID)
void ChromeAutofillClient::SetAutofillSaveCardBottomSheetBridgeForTesting(
    std::unique_ptr<AutofillSaveCardBottomSheetBridge>
        autofill_save_card_bottom_sheet_bridge) {
  autofill_save_card_bottom_sheet_bridge_ =
      std::move(autofill_save_card_bottom_sheet_bridge);
}
#endif

Profile* ChromeAutofillClient::GetProfile() const {
  if (!web_contents())
    return nullptr;
  return Profile::FromBrowserContext(web_contents()->GetBrowserContext());
}

std::u16string ChromeAutofillClient::GetAccountHolderName() {
  Profile* profile = GetProfile();
  if (!profile)
    return std::u16string();
  signin::IdentityManager* identity_manager =
      IdentityManagerFactory::GetForProfile(profile);
  if (!identity_manager)
    return std::u16string();
  AccountInfo primary_account_info = identity_manager->FindExtendedAccountInfo(
      identity_manager->GetPrimaryAccountInfo(signin::ConsentLevel::kSignin));
  return base::UTF8ToUTF16(primary_account_info.full_name);
}

void ChromeAutofillClient::ShowAutofillPopupImpl(
    const PopupOpenArgs& open_args,
    base::WeakPtr<AutofillPopupDelegate> delegate) {
  // Convert element_bounds to be in screen space.
  const gfx::Rect client_area = web_contents()->GetContainerBounds();
  const gfx::RectF element_bounds_in_screen_space =
      open_args.element_bounds + client_area.OffsetFromOrigin();

  // Deletes or reuses the old `popup_controller_`.
  popup_controller_ = AutofillPopupControllerImpl::GetOrCreate(
      popup_controller_, delegate, web_contents(),
      PopupControllerCommon(element_bounds_in_screen_space,
                            open_args.text_direction,
                            web_contents()->GetNativeView()),
      open_args.form_control_ax_id);

  popup_controller_->Show(
      open_args.suggestions, open_args.trigger_source,
      ShouldAutofillPopupAutoselectFirstSuggestion(open_args.trigger_source));

  // When testing, try to keep popup open when the reason to hide is one of:
  // - An external browser frame resize that is extraneous to our testing goals.
  // - Too many fields get focus one after another (for example, multiple
  // password fields being autofilled by default on Desktop).
  if (keep_popup_open_for_testing_ && popup_controller_.get()) {
    popup_controller_->KeepPopupOpenForTesting();
  }
}

base::WeakPtr<ChromeAutofillClient> ChromeAutofillClient::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

#if BUILDFLAG(IS_ANDROID)
AutofillSaveCardBottomSheetBridge*
ChromeAutofillClient::GetOrCreateAutofillSaveCardBottomSheetBridge() {
  if (!autofill_save_card_bottom_sheet_bridge_) {
    // During shutdown the window may be null. There is no need to show the
    // bottom sheet during shutdown.
    auto* window_android = web_contents()->GetTopLevelNativeWindow();
    TabModel* tab_model =
        TabModelList::GetTabModelForWebContents(web_contents());
    if (window_android && tab_model) {
      autofill_save_card_bottom_sheet_bridge_ =
          std::make_unique<AutofillSaveCardBottomSheetBridge>(window_android,
                                                              tab_model);
    }
  }
  return autofill_save_card_bottom_sheet_bridge_.get();
}
#endif

std::unique_ptr<AutofillManager> ChromeAutofillClient::CreateManager(
    base::PassKey<ContentAutofillDriver> pass_key,
    ContentAutofillDriver& driver) {
  return std::make_unique<BrowserAutofillManager>(
      &driver, g_browser_process->GetApplicationLocale());
}

void ChromeAutofillClient::set_test_addresses(
    std::vector<AutofillProfile> test_addresses) {
  test_addresses_ = std::move(test_addresses);
}

base::span<const AutofillProfile> ChromeAutofillClient::GetTestAddresses()
    const {
  return test_addresses_;
}

}  // namespace autofill
