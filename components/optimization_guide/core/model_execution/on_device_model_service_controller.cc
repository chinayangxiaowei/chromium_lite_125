// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/core/model_execution/on_device_model_service_controller.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "base/files/file_path.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/task/thread_pool.h"
#include "components/optimization_guide/core/model_execution/feature_keys.h"
#include "components/optimization_guide/core/model_execution/on_device_model_access_controller.h"
#include "components/optimization_guide/core/model_execution/on_device_model_component.h"
#include "components/optimization_guide/core/model_execution/on_device_model_execution_config_interpreter.h"
#include "components/optimization_guide/core/model_execution/session_impl.h"
#include "components/optimization_guide/core/model_util.h"
#include "components/optimization_guide/core/optimization_guide_constants.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/optimization_guide/core/optimization_guide_switches.h"
#include "components/optimization_guide/core/optimization_guide_util.h"
#include "components/optimization_guide/proto/model_execution.pb.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/on_device_model/public/cpp/model_assets.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"
#include "services/on_device_model/public/mojom/on_device_model_service.mojom.h"

namespace optimization_guide {

namespace {

class ScopedEligibilityReasonLogger {
 public:
  explicit ScopedEligibilityReasonLogger(ModelBasedCapabilityKey feature)
      : feature_(feature) {}
  ~ScopedEligibilityReasonLogger() {
    CHECK_NE(reason_, OnDeviceModelEligibilityReason::kUnknown);
    base::UmaHistogramEnumeration(
        base::StrCat(
            {"OptimizationGuide.ModelExecution.OnDeviceModelEligibilityReason.",
             GetStringNameForModelExecutionFeature(feature_)}),
        reason_);
  }

  void set_reason(OnDeviceModelEligibilityReason reason) { reason_ = reason; }

 private:
  ModelBasedCapabilityKey feature_;

  OnDeviceModelEligibilityReason reason_ =
      OnDeviceModelEligibilityReason::kUnknown;
};

class ScopedTextSafetyModelMetadataValidityLogger {
 public:
  ScopedTextSafetyModelMetadataValidityLogger() = default;
  ~ScopedTextSafetyModelMetadataValidityLogger() {
    CHECK_NE(TextSafetyModelMetadataValidity::kUnknown, validity_);
    base::UmaHistogramEnumeration(
        "OptimizationGuide.ModelExecution."
        "OnDeviceTextSafetyModelMetadataValidity",
        validity_);
  }

  void set_validity(TextSafetyModelMetadataValidity validity) {
    validity_ = validity;
  }

  TextSafetyModelMetadataValidity validity_ =
      TextSafetyModelMetadataValidity::kUnknown;
};

OnDeviceModelLoadResult ConvertToOnDeviceModelLoadResult(
    on_device_model::mojom::LoadModelResult result) {
  switch (result) {
    case on_device_model::mojom::LoadModelResult::kSuccess:
      return OnDeviceModelLoadResult::kSuccess;
    case on_device_model::mojom::LoadModelResult::kGpuBlocked:
      return OnDeviceModelLoadResult::kGpuBlocked;
    case on_device_model::mojom::LoadModelResult::kFailedToLoadLibrary:
      return OnDeviceModelLoadResult::kFailedToLoadLibrary;
  }
}

bool HasRequiredSafetyFiles(const ModelInfo& model_info) {
  return model_info.GetAdditionalFileWithBaseName(kTsDataFile) &&
         model_info.GetAdditionalFileWithBaseName(kTsSpModelFile);
}

}  // namespace

OnDeviceModelServiceController::OnDeviceModelServiceController(
    std::unique_ptr<OnDeviceModelAccessController> access_controller,
    base::WeakPtr<optimization_guide::OnDeviceModelComponentStateManager>
        on_device_component_state_manager)
    : access_controller_(std::move(access_controller)),
      on_device_component_state_manager_(
          std::move(on_device_component_state_manager)),
      config_interpreter_(
          std::make_unique<OnDeviceModelExecutionConfigInterpreter>()) {
  if (on_device_component_state_manager_) {
    on_device_component_state_manager_->AddObserver(this);
  }
}

OnDeviceModelServiceController::~OnDeviceModelServiceController() {
  if (on_device_component_state_manager_) {
    on_device_component_state_manager_->RemoveObserver(this);
  }
}

void OnDeviceModelServiceController::Init() {
  auto model_path_override_switch =
      switches::GetOnDeviceModelExecutionOverride();
  if (model_path_override_switch) {
    SetModelPath(*StringToFilePath(*model_path_override_switch), "override");
  } else if (on_device_component_state_manager_) {
    const OnDeviceModelComponentState* state =
        on_device_component_state_manager_->GetState();
    if (state) {
      SetModelPath(state->GetInstallDirectory(),
                   state->GetComponentVersion().GetString());
    }
  }
}

void OnDeviceModelServiceController::ClearModelPath() {
  model_path_ = std::nullopt;
  model_versions_ = std::nullopt;
  config_interpreter_->ClearState();
  model_remote_.reset();
}

void OnDeviceModelServiceController::SetModelPath(
    const base::FilePath& model_path,
    const std::string& version) {
  // Even if model_path didn't change, we want to go through this process anyway
  // because the content in the directory may have changed.
  ClearModelPath();
  model_path_ = model_path;
  model_versions_ = GetModelVersions(version);
  config_interpreter_->UpdateConfigWithFileDir(model_path);
}

std::unique_ptr<OptimizationGuideModelExecutor::Session>
OnDeviceModelServiceController::CreateSession(
    ModelBasedCapabilityKey feature,
    ExecuteRemoteFn execute_remote_fn,
    base::WeakPtr<OptimizationGuideLogger> optimization_guide_logger,
    base::WeakPtr<ModelQualityLogsUploaderService>
        model_quality_uploader_service,
    const std::optional<SessionConfigParams>& config_params) {
  if (on_device_component_state_manager_) {
    on_device_component_state_manager_->OnDeviceEligibleFeatureUsed();
  }
  ScopedEligibilityReasonLogger logger(feature);
  if (!base::FeatureList::IsEnabled(
          features::kOptimizationGuideOnDeviceModel)) {
    logger.set_reason(OnDeviceModelEligibilityReason::kFeatureNotEnabled);
    return nullptr;
  }
  if (!model_path_) {
    logger.set_reason(OnDeviceModelEligibilityReason::kModelNotAvailable);
    return nullptr;
  }

  on_device_model::ModelAssetPaths model_paths;
  model_paths.sp_model = model_path_->Append(kSpModelFile);
  model_paths.model = model_path_->Append(kModelFile);
  model_paths.weights = model_path_->Append(kWeightsFile);

  std::optional<proto::FeatureTextSafetyConfiguration> safety_config;
  if (!safety_model_info_ && features::GetOnDeviceModelMustUseSafetyModel()) {
    logger.set_reason(OnDeviceModelEligibilityReason::kSafetyModelNotAvailable);
    return nullptr;
  }
  if (safety_model_info_) {
    safety_config =
        safety_model_info_->GetConfig(ToModelExecutionFeatureProto(feature));
    if (!safety_config && features::GetOnDeviceModelMustUseSafetyModel()) {
      logger.set_reason(
          OnDeviceModelEligibilityReason::kSafetyConfigNotAvailableForFeature);
      return nullptr;
    }

    if (safety_config) {
      model_paths.ts_data = safety_model_info_->GetDataPath();
      model_paths.ts_sp_model = safety_model_info_->GetSpModelPath();

      if (!safety_config->allowed_languages().empty()) {
        if (language_detection_model_path_) {
          model_paths.language_detection_model =
              *language_detection_model_path_;
        } else if (features::GetOnDeviceModelMustUseSafetyModel()) {
          logger.set_reason(OnDeviceModelEligibilityReason::
                                kLanguageDetectionModelNotAvailable);
          return nullptr;
        }
      }
    }
  }

  scoped_refptr<const OnDeviceModelFeatureAdapter> adapter =
      config_interpreter_->GetAdapter(ToModelExecutionFeatureProto(feature));
  if (!adapter) {
    logger.set_reason(
        OnDeviceModelEligibilityReason::kConfigNotAvailableForFeature);
    return nullptr;
  }

  if (feature == ModelBasedCapabilityKey::kCompose &&
      !base::FeatureList::IsEnabled(
          features::kOptimizationGuideComposeOnDeviceEval)) {
    logger.set_reason(
        OnDeviceModelEligibilityReason::kFeatureExecutionNotEnabled);
    return nullptr;
  }
  OnDeviceModelEligibilityReason reason =
      access_controller_->ShouldStartNewSession();
  logger.set_reason(reason);
  if (reason != OnDeviceModelEligibilityReason::kSuccess) {
    return nullptr;
  }
  CHECK_EQ(reason, OnDeviceModelEligibilityReason::kSuccess);

  SessionImpl::OnDeviceOptions opts;
  opts.model_client = std::make_unique<OnDeviceModelClient>(
      weak_ptr_factory_.GetWeakPtr(), model_paths);
  opts.model_versions.CopyFrom(model_versions_.value());
  opts.adapter = std::move(adapter);
  opts.safety_cfg = SafetyConfig(safety_config);

  return std::make_unique<SessionImpl>(
      feature, std::move(opts), std::move(execute_remote_fn),
      optimization_guide_logger, model_quality_uploader_service, config_params);
}

void OnDeviceModelServiceController::GetEstimatedPerformanceClass(
    GetEstimatedPerformanceClassCallback callback) {
  LaunchService();
  service_remote_->GetEstimatedPerformanceClass(base::BindOnce(
      [](GetEstimatedPerformanceClassCallback callback,
         on_device_model::mojom::PerformanceClass performance_class) {
        std::move(callback).Run(performance_class);
      },
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  std::nullopt)));
}

mojo::Remote<on_device_model::mojom::OnDeviceModel>&
OnDeviceModelServiceController::GetOrCreateModelRemote(
    on_device_model::ModelAssetPaths model_paths) {
  if (!model_remote_) {
    LaunchService();
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&on_device_model::LoadModelAssets, model_paths),
        base::BindOnce(&OnDeviceModelServiceController::OnModelAssetsLoaded,
                       weak_ptr_factory_.GetWeakPtr(),
                       model_remote_.BindNewPipeAndPassReceiver()));
    model_remote_.set_disconnect_handler(
        base::BindOnce(&OnDeviceModelServiceController::OnDisconnected,
                       base::Unretained(this)));
    model_remote_.set_idle_handler(
        features::GetOnDeviceModelIdleTimeout(),
        base::BindRepeating(&OnDeviceModelServiceController::OnRemoteIdle,
                            base::Unretained(this)));
  }
  return model_remote_;
}

void OnDeviceModelServiceController::OnModelAssetsLoaded(
    mojo::PendingReceiver<on_device_model::mojom::OnDeviceModel> model,
    on_device_model::ModelAssets assets) {
  if (!service_remote_) {
    // Close the files on a background thread.
    base::ThreadPool::PostTask(FROM_HERE, {base::MayBlock()},
                               base::DoNothingWithBoundArgs(std::move(assets)));
    return;
  }
  // TODO(b/302402959): Choose max_tokens based on device.
  int max_tokens = features::GetOnDeviceModelMaxTokensForContext() +
                   features::GetOnDeviceModelMaxTokensForExecute() +
                   features::GetOnDeviceModelMaxTokensForOutput();
  auto params = on_device_model::mojom::LoadModelParams::New();
  params->assets = std::move(assets);
  params->max_tokens = max_tokens;
  if (safety_model_info_) {
    params->ts_dimension = safety_model_info_->num_output_categories();
  }
  service_remote_->LoadModel(
      std::move(params), std::move(model),
      base::BindOnce(&OnDeviceModelServiceController::OnLoadModelResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void OnDeviceModelServiceController::SetLanguageDetectionModel(
    base::optional_ref<const ModelInfo> model_info) {
  if (!model_info.has_value()) {
    language_detection_model_path_.reset();
    return;
  }

  language_detection_model_path_ = model_info->GetModelFilePath();
}

void OnDeviceModelServiceController::MaybeUpdateSafetyModel(
    base::optional_ref<const ModelInfo> model_info) {
  safety_model_info_ = SafetyModelInfo::Load(model_info);
  if (safety_model_info_ && model_versions_) {
    model_versions_->set_text_safety_model_version(
        safety_model_info_->GetVersion());
  }
}

void OnDeviceModelServiceController::StateChanged(
    const OnDeviceModelComponentState* state) {
  if (switches::GetOnDeviceModelExecutionOverride()) {
    return;
  }

  if (state) {
    SetModelPath(state->GetInstallDirectory(),
                 state->GetComponentVersion().GetString());
  } else {
    ClearModelPath();
  }
}

std::unique_ptr<OnDeviceModelServiceController::SafetyModelInfo>
OnDeviceModelServiceController::SafetyModelInfo::Load(
    base::optional_ref<const ModelInfo> opt_model_info) {
  if (!opt_model_info.has_value() || !HasRequiredSafetyFiles(*opt_model_info)) {
    return nullptr;
  }
  const ModelInfo& model_info = *opt_model_info;
  ScopedTextSafetyModelMetadataValidityLogger logger;

  if (!model_info.GetModelMetadata()) {
    logger.set_validity(TextSafetyModelMetadataValidity::kNoMetadata);
    return nullptr;
  }

  std::optional<proto::TextSafetyModelMetadata> model_metadata =
      ParsedAnyMetadata<proto::TextSafetyModelMetadata>(
          *model_info.GetModelMetadata());
  if (!model_metadata) {
    logger.set_validity(TextSafetyModelMetadataValidity::kMetadataWrongType);
    return nullptr;
  }

  logger.set_validity(TextSafetyModelMetadataValidity::kNoFeatureConfigs);

  base::flat_map<proto::ModelExecutionFeature,
                 proto::FeatureTextSafetyConfiguration>
      feature_configs;
  for (const auto& feature_config :
       model_metadata->feature_text_safety_configurations()) {
    logger.set_validity(TextSafetyModelMetadataValidity::kValid);
    feature_configs[feature_config.feature()] = feature_config;
  }

  return base::WrapUnique(
      new SafetyModelInfo(model_info, model_metadata->num_output_categories(),
                          std::move(feature_configs)));
}

void OnDeviceModelServiceController::OnLoadModelResult(
    on_device_model::mojom::LoadModelResult result) {
  base::UmaHistogramEnumeration(
      "OptimizationGuide.ModelExecution.OnDeviceModelLoadResult",
      ConvertToOnDeviceModelLoadResult(result));
  switch (result) {
    case on_device_model::mojom::LoadModelResult::kGpuBlocked:
      access_controller_->OnGpuBlocked();
      model_remote_.reset();
      break;
    case on_device_model::mojom::LoadModelResult::kSuccess:
      break;
    case on_device_model::mojom::LoadModelResult::kFailedToLoadLibrary:
      break;
  }
}

void OnDeviceModelServiceController::OnDisconnected() {
  model_remote_.reset();
  access_controller_->OnDisconnectedFromRemote();
}

void OnDeviceModelServiceController::ShutdownServiceIfNoModelLoaded() {
  if (!model_remote_) {
    service_remote_.reset();
  }
}

void OnDeviceModelServiceController::OnRemoteIdle() {
  service_remote_.reset();
  model_remote_.reset();
}

proto::OnDeviceModelVersions OnDeviceModelServiceController::GetModelVersions(
    const std::string& component_version) const {
  CHECK(!component_version.empty());

  proto::OnDeviceModelVersions versions;
  versions.mutable_on_device_model_service_version()->set_component_version(
      component_version);

  if (safety_model_info_) {
    versions.set_text_safety_model_version(safety_model_info_->GetVersion());
  }

  return versions;
}

OnDeviceModelServiceController::OnDeviceModelClient::OnDeviceModelClient(
    base::WeakPtr<OnDeviceModelServiceController> controller,
    on_device_model::ModelAssetPaths model_paths)
    : controller_(controller), model_paths_(model_paths) {}

OnDeviceModelServiceController::OnDeviceModelClient::~OnDeviceModelClient() =
    default;

bool OnDeviceModelServiceController::OnDeviceModelClient::ShouldUse() {
  return controller_ &&
         controller_->access_controller_->ShouldStartNewSession() ==
             OnDeviceModelEligibilityReason::kSuccess;
}

mojo::Remote<on_device_model::mojom::OnDeviceModel>&
OnDeviceModelServiceController::OnDeviceModelClient::GetModelRemote() {
  return controller_->GetOrCreateModelRemote(model_paths_);
}

void OnDeviceModelServiceController::OnDeviceModelClient::
    OnResponseCompleted() {
  if (controller_) {
    controller_->access_controller_->OnResponseCompleted();
  }
}

void OnDeviceModelServiceController::OnDeviceModelClient::OnSessionTimedOut() {
  if (controller_) {
    controller_->access_controller_->OnSessionTimedOut();
  }
}

std::optional<proto::FeatureTextSafetyConfiguration>
OnDeviceModelServiceController::SafetyModelInfo::GetConfig(
    proto::ModelExecutionFeature feature) const {
  auto it = feature_configs_.find(feature);
  if (it == feature_configs_.end()) {
    return std::nullopt;
  }

  return it->second;
}

base::FilePath OnDeviceModelServiceController::SafetyModelInfo::GetDataPath()
    const {
  return *model_info_.GetAdditionalFileWithBaseName(kTsDataFile);
}

base::FilePath OnDeviceModelServiceController::SafetyModelInfo::GetSpModelPath()
    const {
  return *model_info_.GetAdditionalFileWithBaseName(kTsSpModelFile);
}

int64_t OnDeviceModelServiceController::SafetyModelInfo::GetVersion() const {
  return model_info_.GetVersion();
}

OnDeviceModelServiceController::SafetyModelInfo::SafetyModelInfo(
    const ModelInfo& model_info,
    uint32_t num_output_categories,
    base::flat_map<proto::ModelExecutionFeature,
                   proto::FeatureTextSafetyConfiguration> feature_configs)
    : model_info_(model_info),
      num_output_categories_(num_output_categories),
      feature_configs_(std::move(feature_configs)) {}

OnDeviceModelServiceController::SafetyModelInfo::~SafetyModelInfo() = default;

}  // namespace optimization_guide
