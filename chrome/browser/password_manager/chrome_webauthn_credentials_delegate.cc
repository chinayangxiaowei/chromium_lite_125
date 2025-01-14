// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/chrome_webauthn_credentials_delegate.h"

#include <optional>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "build/build_config.h"
#include "components/password_manager/core/browser/passkey_credential.h"
#include "components/password_manager/core/browser/password_ui_utils.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/webauthn/authenticator_request_scheduler.h"
#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/webauthn/android/webauthn_request_delegate_android.h"
#endif

using password_manager::PasskeyCredential;
using OnPasskeySelectedCallback =
    password_manager::WebAuthnCredentialsDelegate::OnPasskeySelectedCallback;

ChromeWebAuthnCredentialsDelegate::ChromeWebAuthnCredentialsDelegate(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

ChromeWebAuthnCredentialsDelegate::~ChromeWebAuthnCredentialsDelegate() =
    default;

void ChromeWebAuthnCredentialsDelegate::LaunchWebAuthnFlow() {
#if !BUILDFLAG(IS_ANDROID)
  ChromeAuthenticatorRequestDelegate* authenticator_delegate =
      AuthenticatorRequestScheduler::GetRequestDelegate(web_contents_);
  if (!authenticator_delegate) {
    return;
  }
  authenticator_delegate->dialog_controller()
      ->TransitionToModalWebAuthnRequest();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void ChromeWebAuthnCredentialsDelegate::SelectPasskey(
    const std::string& backend_id,
    OnPasskeySelectedCallback callback) {
  // `backend_id` is the base64-encoded credential ID. See `PasskeyCredential`
  // for where these are encoded.
  std::optional<std::vector<uint8_t>> selected_credential_id =
      base::Base64Decode(backend_id);
  DCHECK(selected_credential_id);

#if BUILDFLAG(IS_ANDROID)
  std::move(callback).Run();
  auto* request_delegate =
      WebAuthnRequestDelegateAndroid::GetRequestDelegate(web_contents_);
  if (!request_delegate) {
    return;
  }
  request_delegate->OnWebAuthnAccountSelected(*selected_credential_id);
#else
  ChromeAuthenticatorRequestDelegate* authenticator_delegate =
      AuthenticatorRequestScheduler::GetRequestDelegate(web_contents_);
  if (!authenticator_delegate) {
    std::move(callback).Run();
    return;
  }
  authenticator_delegate->dialog_controller()->OnAccountPreselected(
      *selected_credential_id);
  // TODO(crbug.com/40274370): Update the OnAccountPreselected to run the
  // callback.
  std::move(callback).Run();
#endif  // BUILDFLAG(IS_ANDROID)
}

const std::optional<std::vector<PasskeyCredential>>&
ChromeWebAuthnCredentialsDelegate::GetPasskeys() const {
  return passkeys_;
}

base::WeakPtr<password_manager::WebAuthnCredentialsDelegate>
ChromeWebAuthnCredentialsDelegate::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool ChromeWebAuthnCredentialsDelegate::OfferPasskeysFromAnotherDeviceOption()
    const {
  return offer_passkey_from_another_device_;
}

void ChromeWebAuthnCredentialsDelegate::RetrievePasskeys(
    base::OnceClosure callback) {
  if (passkeys_.has_value()) {
    // Entries were already populated from the WebAuthn request.
    std::move(callback).Run();
    return;
  }

  retrieve_passkeys_callback_ = std::move(callback);
}

void ChromeWebAuthnCredentialsDelegate::OnCredentialsReceived(
    std::vector<PasskeyCredential> credentials,
    bool offer_passkey_from_another_device) {
  passkeys_ = std::move(credentials);
  offer_passkey_from_another_device_ = offer_passkey_from_another_device;
  if (retrieve_passkeys_callback_) {
    std::move(retrieve_passkeys_callback_).Run();
  }
}

void ChromeWebAuthnCredentialsDelegate::NotifyWebAuthnRequestAborted() {
  passkeys_ = std::nullopt;
  if (retrieve_passkeys_callback_) {
    std::move(retrieve_passkeys_callback_).Run();
  }
}

#if BUILDFLAG(IS_ANDROID)
void ChromeWebAuthnCredentialsDelegate::ShowAndroidHybridSignIn() {
  if (WebAuthnRequestDelegateAndroid* delegate =
          WebAuthnRequestDelegateAndroid::GetRequestDelegate(web_contents_)) {
    delegate->ShowHybridSignIn();
  }
}

bool ChromeWebAuthnCredentialsDelegate::IsAndroidHybridAvailable() const {
  return android_hybrid_available_.value();
}

void ChromeWebAuthnCredentialsDelegate::SetAndroidHybridAvailable(
    AndroidHybridAvailable available) {
  android_hybrid_available_ = available;
}
#endif
