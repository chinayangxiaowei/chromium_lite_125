// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/model_execution/model_manager_impl.h"

#include "chrome/browser/model_execution/model_execution_session.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/optimization_guide/core/model_execution/feature_keys.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "third_party/blink/public/mojom/model_execution/model_manager.mojom.h"

DOCUMENT_USER_DATA_KEY_IMPL(ModelManagerImpl);

ModelManagerImpl::ModelManagerImpl(content::RenderFrameHost* rfh)
    : DocumentUserData<ModelManagerImpl>(rfh) {
  browser_context_ = rfh->GetBrowserContext()->GetWeakPtr();
}

ModelManagerImpl::~ModelManagerImpl() = default;

// static
void ModelManagerImpl::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<blink::mojom::ModelManager> receiver) {
  ModelManagerImpl* model_manager =
      ModelManagerImpl::GetOrCreateForCurrentDocument(render_frame_host);
  model_manager->receiver_.Bind(std::move(receiver));
}

void ModelManagerImpl::CanCreateGenericSession(
    CanCreateGenericSessionCallback callback) {
  // TODO(leimy): add the checks after optimization guide component provide more
  // method to determine if a session could be started.
  content::BrowserContext* browser_context = browser_context_.get();
  std::move(callback).Run(
      /*can_create=*/browser_context &&
      !!OptimizationGuideKeyedServiceFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context)));
}

void ModelManagerImpl::CreateGenericSession(
    mojo::PendingReceiver<blink::mojom::ModelGenericSession> receiver,
    blink::mojom::ModelGenericSessionSamplingParamsPtr sampling_params,
    CreateGenericSessionCallback callback) {
  content::BrowserContext* browser_context = browser_context_.get();
  if (!browser_context) {
    receiver_.ReportBadMessage(
        "Caller should ensure `CanStartModelExecutionSession()` "
        "returns true before calling this method.");
    std::move(callback).Run(/*success=*/false);
    return;
  }

  OptimizationGuideKeyedService* service =
      OptimizationGuideKeyedServiceFactory::GetForProfile(
          Profile::FromBrowserContext(browser_context));
  if (!service) {
    receiver_.ReportBadMessage(
        "Caller should ensure `CanStartModelExecutionSession()` "
        "returns true before calling this method.");
    std::move(callback).Run(/*success=*/false);
    return;
  }

  optimization_guide::SessionConfigParams config_params =
      optimization_guide::SessionConfigParams{.disable_server_fallback = true};
  if (sampling_params) {
    config_params.sampling_params = optimization_guide::SamplingParams{
        .top_k = sampling_params->top_k,
        .temperature = sampling_params->temperature};
  }

  std::unique_ptr<optimization_guide::OptimizationGuideModelExecutor::Session>
      session = service->StartSession(
          optimization_guide::ModelBasedCapabilityKey::kTest, config_params);
  // TODO(leimy): after this check is done by optimization guide and we can
  // return that from `CanStartModelExecutionSession()`, we should replace this
  // block by a CHECK, and stop returning any boolean value from this method.
  if (!session) {
    std::move(callback).Run(/*success=*/false);
    return;
  }
  // The new `ModelExecutionSession` shares the same lifetime with the
  // `receiver`.
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ModelExecutionSession>(std::move(session)),
      std::move(receiver));
  std::move(callback).Run(/*success=*/true);
}

void ModelManagerImpl::GetDefaultGenericSessionSamplingParams(
    GetDefaultGenericSessionSamplingParamsCallback callback) {
  std::move(callback).Run(blink::mojom::ModelGenericSessionSamplingParams::New(
      optimization_guide::features::GetOnDeviceModelDefaultTopK(),
      optimization_guide::features::GetOnDeviceModelDefaultTemperature()));
}
