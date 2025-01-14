// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/network/network_portal_signin_window.h"

#include "chrome/browser/ash/net/network_portal_detector_test_impl.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/ash/components/network/portal_detector/network_portal_detector.h"
#include "components/captive_portal/content/captive_portal_tab_helper.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chromeos {

class NetworkPortalSigninWindowAshBrowserTest : public InProcessBrowserTest {
 public:
  NetworkPortalSigninWindowAshBrowserTest() {
    ash::network_portal_detector::InitializeForTesting(
        new ash::NetworkPortalDetectorTestImpl());
  }
  ~NetworkPortalSigninWindowAshBrowserTest() override = default;

  ash::NetworkPortalDetectorTestImpl* network_portal_detector() {
    return static_cast<ash::NetworkPortalDetectorTestImpl*>(
        ash::network_portal_detector::GetInstance());
  }
};

IN_PROC_BROWSER_TEST_F(NetworkPortalSigninWindowAshBrowserTest,
                       RequestPortalDetection) {
  content::CreateAndLoadWebContentsObserver web_contents_observer;

  NetworkPortalSigninWindow::Get()->Show(
      GURL("http://www.gstatic.com/generate_204"));
  ASSERT_TRUE(NetworkPortalSigninWindow::Get()->GetBrowserForTesting());

  web_contents_observer.Wait();

  // Showing the window should generate a DidFinishNavigation event which should
  // trigger a corresponding captive portal detection request.
  EXPECT_EQ(network_portal_detector()->captive_portal_detection_requested(), 1);

  // The popup window sets the |is_captive_portal_popup| param which should
  // set the |CaptivePortalTabHelper::is_captive_portal_window| property.
  content::WebContents* web_contents =
      NetworkPortalSigninWindow::Get()->GetWebContentsForTesting();
  ASSERT_TRUE(web_contents);
  captive_portal::CaptivePortalTabHelper* helper =
      captive_portal::CaptivePortalTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(helper);
  EXPECT_TRUE(helper->is_captive_portal_window());
}

}  // namespace chromeos
