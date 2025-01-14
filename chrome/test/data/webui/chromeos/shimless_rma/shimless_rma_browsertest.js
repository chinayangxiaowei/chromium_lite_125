// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for chrome://shimless-rma. Tests
 * individual polymer components in isolation. To run all tests in a single
 * instance (default, faster):
 * `browser_tests --gtest_filter=ShimlessRmaApp*`
 * To run a single test suite such as 'AllInputsDisabledTest':
 * browser_tests --gtest_filter=ShimlessRmaAppAllInputsDisabledTest.All
 */

GEN_INCLUDE(['//chrome/test/data/webui/chromeos/polymer_browser_test_base.js']);

GEN('#include "ash/constants/ash_features.h"');
GEN('#include "content/public/test/browser_test.h"');

this.ShimlessRmaBrowserTest = class extends PolymerTest {
  /** @override */
  get featureList() {
    return {
      enabled: [
        'ash::features::kShimlessRMAOsUpdate',
      ],
    };
  }

  /** @override */
  get commandLineSwitches() {
    return [{switchName: 'launch-rma'}];
  }
};

const tests = [
  [
    'ReimagingCalibrationSetupPageTest',
    'reimaging_calibration_setup_page_test.js'
  ],
  ['ReimagingFirmwareUpdatePageTest', 'reimaging_firmware_update_page_test.js'],
  [
    'ReimagingDeviceInformationPageTest',
    'reimaging_device_information_page_test.js'
  ],
  ['ReimagingProvisioningPageTest', 'reimaging_provisioning_page_test.js'],
  ['RepairComponentChipTest', 'repair_component_chip_test.js'],
  ['Shimless3pDiagTest', 'shimless_3p_diag_test.js'],
  ['ShimlessRMAAppTest', 'shimless_rma_app_test.js'],
  ['WrapupFinalizePageTest', 'wrapup_finalize_page_test.js'],
  ['WrapupRepairCompletePageTest', 'wrapup_repair_complete_page_test.js'],
  ['WrapupRestockPageTest', 'wrapup_restock_page_test.js'],
  [
    'WrapupWaitForManualWpEnablePageTest',
    'wrapup_wait_for_manual_wp_enable_page_test.js'
  ],
];

tests.forEach(test => registerTest(...test));

/*
 * Add a `caseName` to a specific test to disable it i.e. 'DISABLED_All'
 * @param {string} testName
 * @param {string} module
 * @param {string} caseName
 */
function registerTest(testName, module, caseName) {
  const className = `ShimlessRmaApp${testName}`;
  this[className] = class extends ShimlessRmaBrowserTest {
    /** @override */
    get browsePreload() {
      return `chrome://shimless-rma/test_loader.html` +
          `?module=chromeos/shimless_rma/${module}`;
    }
  };
  TEST_F(className, caseName || 'All', () => mocha.run());
}
