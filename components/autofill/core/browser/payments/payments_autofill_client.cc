// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/payments_autofill_client.h"

#include "base/functional/callback.h"
#include "components/autofill/core/browser/autofill_progress_dialog_type.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/payments/autofill_error_dialog_context.h"
#include "components/autofill/core/browser/payments/card_unmask_delegate.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_prompt_options.h"

namespace autofill::payments {

PaymentsAutofillClient::~PaymentsAutofillClient() = default;

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
void PaymentsAutofillClient::ShowLocalCardMigrationDialog(
    base::OnceClosure show_migration_dialog_closure) {}

void PaymentsAutofillClient::ConfirmMigrateLocalCardToCloud(
    const LegalMessageLines& legal_message_lines,
    const std::string& user_email,
    const std::vector<MigratableCreditCard>& migratable_credit_cards,
    LocalCardMigrationCallback start_migrating_cards_callback) {}

void PaymentsAutofillClient::ShowLocalCardMigrationResults(
    bool has_server_error,
    const std::u16string& tip_message,
    const std::vector<MigratableCreditCard>& migratable_credit_cards,
    MigrationDeleteCardCallback delete_local_card_callback) {}

void PaymentsAutofillClient::VirtualCardEnrollCompleted(bool is_vcn_enrolled) {}
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

void PaymentsAutofillClient::CreditCardUploadCompleted(bool card_saved) {}

bool PaymentsAutofillClient::IsSaveCardPromptVisible() const {
  return false;
}

void PaymentsAutofillClient::HideSaveCardPromptPrompt() {}

void PaymentsAutofillClient::ShowAutofillProgressDialog(
    AutofillProgressDialogType autofill_progress_dialog_type,
    base::OnceClosure cancel_callback) {}

void PaymentsAutofillClient::CloseAutofillProgressDialog(
    bool show_confirmation_before_closing,
    base::OnceClosure no_interactive_authentication_callback) {}

void PaymentsAutofillClient::ShowCardUnmaskOtpInputDialog(
    const CardUnmaskChallengeOption& challenge_option,
    base::WeakPtr<OtpUnmaskDelegate> delegate) {}

void PaymentsAutofillClient::OnUnmaskOtpVerificationResult(
    OtpUnmaskResult unmask_result) {}

PaymentsNetworkInterface*
PaymentsAutofillClient::GetPaymentsNetworkInterface() {
  return nullptr;
}

void PaymentsAutofillClient::ShowAutofillErrorDialog(
    AutofillErrorDialogContext context) {}

PaymentsWindowManager* PaymentsAutofillClient::GetPaymentsWindowManager() {
  return nullptr;
}

void PaymentsAutofillClient::ShowUnmaskPrompt(
    const CreditCard& card,
    const CardUnmaskPromptOptions& card_unmask_prompt_options,
    base::WeakPtr<CardUnmaskDelegate> delegate) {}

void PaymentsAutofillClient::OnUnmaskVerificationResult(
    AutofillClient::PaymentsRpcResult result) {}

}  // namespace autofill::payments
