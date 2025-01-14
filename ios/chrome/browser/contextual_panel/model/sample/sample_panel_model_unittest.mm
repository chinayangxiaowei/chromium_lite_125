// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/contextual_panel/model/sample/sample_panel_model.h"

#import "base/test/task_environment.h"
#import "ios/chrome/browser/contextual_panel/model/contextual_panel_item_configuration.h"
#import "ios/chrome/browser/contextual_panel/model/sample/sample_panel_item_configuration.h"
#import "testing/platform_test.h"

// Unittests related to the SamplePanelModel.
class SamplePanelModelTest : public PlatformTest {
 public:
  SamplePanelModelTest() {}
  ~SamplePanelModelTest() override {}

  void SetUp() override {
    sample_panel_model_ = std::make_unique<SamplePanelModel>();
  }

  void FetchConfigurationCallback(
      std::unique_ptr<ContextualPanelItemConfiguration> configuration) {
    returned_configuration_ = std::move(configuration);
  }

  base::test::SingleThreadTaskEnvironment task_environment_;

  std::unique_ptr<SamplePanelModel> sample_panel_model_;
  std::unique_ptr<ContextualPanelItemConfiguration> returned_configuration_;
};

// Tests that fetching the configuration for the sample panel model returns.
TEST_F(SamplePanelModelTest, TestFetchConfiguration) {
  base::RunLoop run_loop;

  sample_panel_model_->FetchConfigurationForWebState(
      nullptr, base::BindOnce(&SamplePanelModelTest::FetchConfigurationCallback,
                              base::Unretained(this))
                   .Then(run_loop.QuitClosure()));
  run_loop.Run();

  ASSERT_TRUE(returned_configuration_);
  SamplePanelItemConfiguration* config =
      static_cast<SamplePanelItemConfiguration*>(returned_configuration_.get());
  EXPECT_EQ("sample_config", config->sample_name);
  EXPECT_EQ("Large entrypoint", config->entrypoint_message);
  EXPECT_EQ("chrome_product", config->entrypoint_image_name);
  EXPECT_EQ("Sample entrypoint", config->accessibility_label);
  EXPECT_EQ(ContextualPanelItemConfiguration::high_relevance,
            config->relevance);
  EXPECT_EQ(ContextualPanelItemConfiguration::EntrypointImageType::SFSymbol,
            config->image_type);
}
