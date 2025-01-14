// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/facilitated_payments/core/browser/payments/facilitated_payments_network_interface.h"

#include "components/facilitated_payments/core/browser/payments/facilitated_payments_initiate_payment_request.h"

namespace payments::facilitated {

FacilitatedPaymentsNetworkInterface::FacilitatedPaymentsNetworkInterface(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    signin::IdentityManager* identity_manager,
    autofill::AccountInfoGetter* account_info_getter,
    bool is_off_the_record)
    : PaymentsNetworkInterfaceBase(url_loader_factory,
                                   identity_manager,
                                   account_info_getter,
                                   is_off_the_record) {}

FacilitatedPaymentsNetworkInterface::~FacilitatedPaymentsNetworkInterface() =
    default;

void FacilitatedPaymentsNetworkInterface::InitiatePayment(
    std::unique_ptr<FacilitatedPaymentsInitiatePaymentRequestDetails>
        request_details,
    InitiatePaymentResponseCallback response_callback) {
  IssueRequest(std::make_unique<FacilitatedPaymentsInitiatePaymentRequest>(
      std::move(request_details), std::move(response_callback)));
}

}  // namespace payments::facilitated
