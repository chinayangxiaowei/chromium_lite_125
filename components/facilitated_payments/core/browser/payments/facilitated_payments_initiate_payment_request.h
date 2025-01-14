// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_PAYMENTS_FACILITATED_PAYMENTS_INITIATE_PAYMENT_REQUEST_H_
#define COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_PAYMENTS_FACILITATED_PAYMENTS_INITIATE_PAYMENT_REQUEST_H_

#include <memory>

#include "components/autofill/core/browser/payments/payments_requests/payments_request.h"
#include "components/facilitated_payments/core/browser/payments/facilitated_payments_initiate_payment_request_details.h"
#include "components/facilitated_payments/core/browser/payments/facilitated_payments_initiate_payment_response_details.h"
#include "components/facilitated_payments/core/browser/payments/facilitated_payments_network_interface.h"

namespace payments::facilitated {

// This class is used for making a payment request to the Payments server. It is
// used by all FOPs under Facilitated Payments. It encapsulates the info
// required for making the server call, and pipes the server response back to
// the `FacilitatedPaymentsManager` through a callback.
class FacilitatedPaymentsInitiatePaymentRequest
    : public autofill::payments::PaymentsRequest {
 public:
  FacilitatedPaymentsInitiatePaymentRequest(
      std::unique_ptr<FacilitatedPaymentsInitiatePaymentRequestDetails>
          request_details,
      FacilitatedPaymentsNetworkInterface::InitiatePaymentResponseCallback
          response_callback);
  FacilitatedPaymentsInitiatePaymentRequest(
      const FacilitatedPaymentsInitiatePaymentRequest&) = delete;
  FacilitatedPaymentsInitiatePaymentRequest& operator=(
      const FacilitatedPaymentsInitiatePaymentRequest&) = delete;
  ~FacilitatedPaymentsInitiatePaymentRequest() override;

  // PaymentsRequest:
  std::string GetRequestUrlPath() override;
  std::string GetRequestContentType() override;
  std::string GetRequestContent() override;
  void ParseResponse(const base::Value::Dict& response) override;
  bool IsResponseComplete() override;
  void RespondToDelegate(
      autofill::AutofillClient::PaymentsRpcResult result) override;

 private:
  std::unique_ptr<FacilitatedPaymentsInitiatePaymentRequestDetails>
      request_details_;
  std::unique_ptr<FacilitatedPaymentsInitiatePaymentResponseDetails>
      response_details_;
  FacilitatedPaymentsNetworkInterface::InitiatePaymentResponseCallback
      response_callback_;
};

}  // namespace payments::facilitated

#endif  // COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_PAYMENTS_FACILITATED_PAYMENTS_INITIATE_PAYMENT_REQUEST_H_
