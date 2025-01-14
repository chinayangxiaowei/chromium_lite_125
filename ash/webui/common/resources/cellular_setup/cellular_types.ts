// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Constants used in cellular setup flow.
 */

export enum CellularSetupPageName {
  ESIM_FLOW_UI = 'esim-flow-ui',
  PSIM_FLOW_UI = 'psim-flow-ui',
}

export enum ButtonState {
  ENABLED = 1,
  DISABLED = 2,
  HIDDEN = 3,
}

export enum Button {
  BACKWARD = 1,
  CANCEL = 2,
  FORWARD = 3,
}

export interface ButtonBarState {
  backward?: ButtonState;
  cancel?: ButtonState;
  forward?: ButtonState;
}
