// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/content/browser/content_autofill_driver_factory_test_api.h"
#include "components/autofill/content/browser/test_autofill_client_injector.h"
#include "components/autofill/content/browser/test_content_autofill_client.h"
#include "components/autofill/content/browser/test_content_autofill_driver.h"
#include "components/autofill/core/common/autofill_features.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/back_forward_cache_util.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/features.h"

using testing::_;
using testing::AtLeast;
using testing::Between;
using testing::Ref;

namespace autofill {

namespace {

class MockContentAutofillDriverFactoryObserver
    : public ContentAutofillDriverFactory::Observer {
 public:
  MOCK_METHOD(void,
              OnContentAutofillDriverFactoryDestroyed,
              (ContentAutofillDriverFactory & factory),
              (override));
  MOCK_METHOD(void,
              OnContentAutofillDriverCreated,
              (ContentAutofillDriverFactory & factory,
               ContentAutofillDriver& driver),
              (override));
  MOCK_METHOD(void,
              OnContentAutofillDriverWillBeDeleted,
              (ContentAutofillDriverFactory & factory,
               ContentAutofillDriver& driver),
              (override));
};

}  // namespace

// Test case with one frame.
class ContentAutofillDriverFactoryTest
    : public content::RenderViewHostTestHarness {
 public:
  TestContentAutofillClient* client() {
    return autofill_client_injector_[web_contents()];
  }

  ContentAutofillDriverFactory& factory() {
    return *client()->GetAutofillDriverFactory();
  }

  void NavigateMainFrame(std::string_view url) {
    content::NavigationSimulator::CreateBrowserInitiated(GURL(url),
                                                         web_contents())
        ->Commit();
  }

 protected:
  TestAutofillClientInjector<TestContentAutofillClient>
      autofill_client_injector_;
};

TEST_F(ContentAutofillDriverFactoryTest, MainDriver) {
  NavigateMainFrame("https://a.com/");
  ContentAutofillDriver* main_driver =
      test_api(factory()).GetDriver(main_rfh());
  EXPECT_TRUE(main_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);
  EXPECT_EQ(factory().DriverForFrame(main_rfh()), main_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);
  EXPECT_EQ(factory().DriverForFrame(main_rfh()), main_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);
}

// Test case with two frames: the main frame and one child frame.
class ContentAutofillDriverFactoryTest_WithTwoFrames
    : public ContentAutofillDriverFactoryTest {
 public:
  void NavigateChildFrame(std::string_view url) {
    CHECK(main_rfh());
    if (!child_rfh()) {
      child_rfh_id_ = content::RenderFrameHostTester::For(main_rfh())
                          ->AppendChild(std::string("child"))
                          ->GetGlobalId();
    }
    child_rfh_id_ = content::NavigationSimulator::NavigateAndCommitFromDocument(
                        GURL(url), child_rfh())
                        ->GetGlobalId();
  }

  content::RenderFrameHost* child_rfh() {
    return content::RenderFrameHost::FromID(child_rfh_id_);
  }

 private:
  content::GlobalRenderFrameHostId child_rfh_id_;
};

TEST_F(ContentAutofillDriverFactoryTest_WithTwoFrames, TwoDrivers) {
  NavigateMainFrame("https://a.com/");
  NavigateChildFrame("https://b.com/");
  ASSERT_TRUE(main_rfh());
  ASSERT_TRUE(child_rfh());
  ContentAutofillDriver* main_driver = factory().DriverForFrame(main_rfh());
  ContentAutofillDriver* child_driver = factory().DriverForFrame(child_rfh());
  EXPECT_TRUE(main_driver);
  EXPECT_TRUE(child_driver);
  EXPECT_EQ(factory().DriverForFrame(main_rfh()), main_driver);
  EXPECT_EQ(factory().DriverForFrame(child_rfh()), child_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 2u);
  EXPECT_EQ(factory().DriverForFrame(main_rfh()), main_driver);
  EXPECT_EQ(factory().DriverForFrame(child_rfh()), child_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 2u);
}

// Test case with two frames, where the parameter selects one of them.
class ContentAutofillDriverFactoryTest_WithTwoFrames_PickOne
    : public ContentAutofillDriverFactoryTest_WithTwoFrames,
      public ::testing::WithParamInterface<bool> {
 public:
  content::RenderFrameHost* picked_rfh() {
    return GetParam() ? main_rfh() : child_rfh();
  }
};

INSTANTIATE_TEST_SUITE_P(ContentAutofillDriverFactoryTest,
                         ContentAutofillDriverFactoryTest_WithTwoFrames_PickOne,
                         testing::Bool());

// Tests that a driver is removed in RenderFrameDeleted().
TEST_P(ContentAutofillDriverFactoryTest_WithTwoFrames_PickOne,
       RenderFrameDeleted) {
  NavigateMainFrame("https://a.com/");
  NavigateChildFrame("https://b.com/");
  ASSERT_TRUE(picked_rfh() == main_rfh() || picked_rfh() == child_rfh());
  ContentAutofillDriver* main_driver = factory().DriverForFrame(main_rfh());
  ContentAutofillDriver* child_driver = factory().DriverForFrame(child_rfh());
  EXPECT_TRUE(main_driver);
  EXPECT_TRUE(child_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 2u);
  factory().RenderFrameDeleted(picked_rfh());
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);
  if (picked_rfh() == main_rfh())
    EXPECT_EQ(test_api(factory()).GetDriver(child_rfh()), child_driver);
  else
    EXPECT_EQ(test_api(factory()).GetDriver(main_rfh()), main_driver);
}

// Test case with one frame, with BFcache enabled or disabled depending on the
// parameter.
class ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes
    : public ContentAutofillDriverFactoryTest,
      public ::testing::WithParamInterface<std::tuple<bool>> {
 public:
  ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes() {
    std::vector<base::test::FeatureRef> enabled;
    // Allow BackForwardCache for all devices regardless of their memory.
    std::vector<base::test::FeatureRef> disabled =
        content::GetDefaultDisabledBackForwardCacheFeaturesForTesting();
    (use_bfcache() ? enabled : disabled)
        .push_back(::features::kBackForwardCache);
    scoped_feature_list_.InitWithFeatures(enabled, disabled);
  }

  bool use_bfcache() { return std::get<0>(GetParam()); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    ContentAutofillDriverFactoryTest,
    ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes,
    testing::Combine(testing::Bool()));

// Tests that that a same-documentation navigation does not touch the factory's
// router.
TEST_P(ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes,
       SameDocumentNavigation) {
  NavigateMainFrame("https://a.com/");
  content::RenderFrameHost* orig_rfh = main_rfh();
  ContentAutofillDriver* orig_driver = factory().DriverForFrame(orig_rfh);
  NavigateMainFrame("https://a.com/#same-site");
  ASSERT_EQ(orig_rfh, main_rfh());
  EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), orig_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);

  // TODO(crbug.com/1200511): Test that |router_| has been untouched. To this
  // end, call `orig_driver->FormsSeen({FormData{}})` above and then check
  // here that the router still knows that form. For this to work, we need mock
  // AutofillManagers instead of real BrowserAutofillManager, which are blocked
  // by ContentAutofillDriver's use of the factory callback.
}

// Tests that that a driver is 1:1 with RenderFrameHost, which might or might
// not change after a same-origin navigation.
TEST_P(ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes,
       SameOriginNavigation) {
  NavigateMainFrame("https://a.com/");
  content::RenderFrameHost* orig_rfh = main_rfh();
  ContentAutofillDriver* orig_driver = factory().DriverForFrame(orig_rfh);

  // TODO(crbug.com/1200511): Use mock AutofillManagers and expect a call of
  // AutofillManager::Reset(), which is blocked by ContentAutofillDriver's use
  // of the factory callback.

  NavigateMainFrame("https://a.com/after-navigation");
  // If the RenderFrameHost changed, a new driver for main_rfh() is created, and
  // if BFCache is disabled the driver for |orig_rfh| has now been removed in
  // ContentAutofillDriverFactory::RenderFrameDeleted().
  if (use_bfcache()) {
    EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), orig_driver);
  } else if (main_rfh() != orig_rfh) {
    EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), nullptr);
  }
  EXPECT_EQ(test_api(factory()).num_drivers(), use_bfcache() ? 2u : 1u);
}

// Tests that that a driver is removed and replaced with a fresh one after a
// cross-origin navigation.
TEST_P(ContentAutofillDriverFactoryTest_WithOrWithoutBfCacheAndIframes,
       CrossOriginNavigation) {
  NavigateMainFrame("https://a.com/");
  content::RenderFrameHost* orig_rfh = main_rfh();
  content::GlobalRenderFrameHostId orig_rfh_id = orig_rfh->GetGlobalId();
  ContentAutofillDriver* orig_driver = factory().DriverForFrame(orig_rfh);

  ASSERT_EQ(orig_rfh, main_rfh());
  EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), orig_driver);
  EXPECT_EQ(test_api(factory()).num_drivers(), 1u);

  NavigateMainFrame("https://different-origin-after-navigation.com/");

  ASSERT_NE(orig_rfh_id, main_rfh()->GetGlobalId());
  // A new driver for main_rfh() has been created and the |orig_rfh| has now
  // been removed in ContentAutofillDriverFactory::RenderFrameDeleted(), unless
  // BFcache is enabled (or main_rfh() happens to have the same address as
  // |orig_rfh|).
  if (use_bfcache())
    EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), orig_driver);
  else if (main_rfh() != orig_rfh)
    EXPECT_EQ(test_api(factory()).GetDriver(orig_rfh), nullptr);
  EXPECT_NE(test_api(factory()).GetDriver(main_rfh()), nullptr);
  EXPECT_EQ(test_api(factory()).num_drivers(), use_bfcache() ? 2u : 1u);
}

// Fixture for testing that Autofill is enabled in fenced frames.
class ContentAutofillDriverFactoryTest_FencedFrames
    : public ContentAutofillDriverFactoryTest {
 public:
  ContentAutofillDriverFactoryTest_FencedFrames() {
    std::vector<base::test::FeatureRefAndParams> enabled{
        {blink::features::kFencedFrames, {{"implementation_type", "mparch"}}},
        {blink::features::kFencedFramesAPIChanges, {}}};
    scoped_feature_list_.InitWithFeaturesAndParameters(enabled, {});
  }

  ~ContentAutofillDriverFactoryTest_FencedFrames() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ContentAutofillDriverFactoryTest_FencedFrames,
       DisableAutofillWithinFencedFrame) {
  NavigateMainFrame("http://test.org");
  content::RenderFrameHost* fenced_frame_root =
      content::RenderFrameHostTester::For(main_rfh())->AppendFencedFrame();
  content::RenderFrameHost* fenced_frame_subframe =
      content::RenderFrameHostTester::For(fenced_frame_root)
          ->AppendChild("iframe");
  EXPECT_NE(nullptr, factory().DriverForFrame(main_rfh()));
  EXPECT_NE(nullptr, factory().DriverForFrame(fenced_frame_root));
  EXPECT_NE(nullptr, factory().DriverForFrame(fenced_frame_subframe));
}

// Tests the notifications of ContentAutofillDriverFactory::Observer.
class ContentAutofillDriverFactoryTest_Observer
    : public ContentAutofillDriverFactoryTest {
 public:
  void SetUp() override {
    ContentAutofillDriverFactoryTest::SetUp();
    factory().AddObserver(&observer_);
  }

  void TearDown() override {
    if (client()) {
      factory().RemoveObserver(&observer_);
    }
    ContentAutofillDriverFactoryTest::TearDown();
  }

  MockContentAutofillDriverFactoryObserver observer_;
};

auto IsKnownDriver(ContentAutofillDriverFactory* factory) {
  return testing::Truly([factory](ContentAutofillDriver& driver) {
    return factory->DriverForFrame(driver.render_frame_host()) == &driver;
  });
}

TEST_F(ContentAutofillDriverFactoryTest_Observer, FactoryDestroyed) {
  EXPECT_CALL(observer_,
              OnContentAutofillDriverFactoryDestroyed(Ref(factory())));
  DeleteContents();
}

TEST_F(ContentAutofillDriverFactoryTest_Observer, DriverCreated) {
  EXPECT_CALL(observer_, OnContentAutofillDriverCreated(
                             Ref(factory()), IsKnownDriver(&factory())));
  NavigateMainFrame("https://a.com/");
}

TEST_F(ContentAutofillDriverFactoryTest_Observer, DriverDeleted) {
  EXPECT_CALL(observer_, OnContentAutofillDriverCreated);
  EXPECT_CALL(observer_, OnContentAutofillDriverWillBeDeleted(
                             Ref(factory()), IsKnownDriver(&factory())));
  NavigateMainFrame("https://a.com/");
  factory().RenderFrameDeleted(main_rfh());
}

}  // namespace autofill
