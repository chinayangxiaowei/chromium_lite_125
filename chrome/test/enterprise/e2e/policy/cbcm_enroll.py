# Copyright 2019 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from absl import app
import time
from selenium import webdriver
from selenium.webdriver.common.by import By

from test_util import create_chrome_webdriver
from test_util import getElementFromShadowRoot


def main(argv):
  options = webdriver.ChromeOptions()
  os.environ["CHROME_LOG_FILE"] = r"c:\temp\chrome_log.txt"

  # Flag which tells Chrome to send events to our test endpoint.
  # Debugging tip: this flag only works for Dev and Canary builds.
  # In Stable and Beta builds, Chrome sends events to the default
  # production endpoint: https://chromereporting-pa.googleapis.com/v1/test/events
  options.add_argument(
      "--encrypted-reporting-url=https://autopush-chromereporting-pa.sandbox.googleapis.com/v1/record"
  )

  # This flag tells Chrome to send heartbeat events on start up.
  options.add_argument(
      "--enable-features=EncryptedReportingManualTestHeartbeatEvent,EncryptedReportingPipeline"
  )

  driver = create_chrome_webdriver(chrome_options=options)

  # Give some time for browser to enroll and to send heartbeat events.
  time.sleep(25)

  try:
    # Verify Policy status legend in chrome://policy page
    policy_url = "chrome://policy"
    driver.get(policy_url)
    driver.find_element(By.ID, 'reload-policies').click
    # Give the page 2 seconds to render the legend
    time.sleep(2)
    status_box = driver.find_element(By.CSS_SELECTOR, "status-box")
    el = getElementFromShadowRoot(driver, status_box, ".status-box-fields")

    print(el.find_element(By.CLASS_NAME, 'status-box-heading').text)
    print(el.find_element(By.CLASS_NAME, 'machine-enrollment-name').text)
    print(el.find_element(By.CLASS_NAME, 'machine-enrollment-token').text)
    print(el.find_element(By.CLASS_NAME, 'status').text)
    device_id = el.find_element(By.CLASS_NAME,
                                'machine-enrollment-device-id').text
    print("DEVICE_ID=" + device_id.strip())
  except Exception as error:
    print(error)
  finally:
    driver.quit()


if __name__ == '__main__':
  app.run(main)
