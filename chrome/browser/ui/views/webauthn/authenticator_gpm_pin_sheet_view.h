// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_WEBAUTHN_AUTHENTICATOR_GPM_PIN_SHEET_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_WEBAUTHN_AUTHENTICATOR_GPM_PIN_SHEET_VIEW_H_

#include "chrome/browser/ui/views/webauthn/authenticator_gpm_pin_view.h"
#include "chrome/browser/ui/views/webauthn/authenticator_request_sheet_view.h"
#include "chrome/browser/ui/webauthn/sheet_models.h"

// Represents a sheet in the Web Authentication request dialog that allows the
// user to enter GPM pin code used in passkeys flow.
class AuthenticatorGpmPinSheetView : public AuthenticatorRequestSheetView,
                                     public AuthenticatorGPMPinView::Delegate {
 public:
  explicit AuthenticatorGpmPinSheetView(
      std::unique_ptr<AuthenticatorGPMPinSheetModel> sheet_model);

  AuthenticatorGpmPinSheetView(const AuthenticatorGpmPinSheetView&) = delete;
  AuthenticatorGpmPinSheetView& operator=(const AuthenticatorGpmPinSheetView&) =
      delete;

  ~AuthenticatorGpmPinSheetView() override;

 private:
  AuthenticatorGPMPinSheetModel* gpm_pin_sheet_model();

  // AuthenticatorRequestSheetView:
  std::pair<std::unique_ptr<views::View>, AutoFocus> BuildStepSpecificContent()
      override;

  // AuthenticatorGPMPinView::Delegate:
  void OnPinChanged(std::u16string pin) override;
};

#endif  // CHROME_BROWSER_UI_VIEWS_WEBAUTHN_AUTHENTICATOR_GPM_PIN_SHEET_VIEW_H_
