// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview Polymer element for theme selection screen.
 */
import '//resources/ash/common/cr_elements/cros_color_overrides.css.js';
import '//resources/ash/common/cr_elements/cr_radio_button/cr_radio_button.js';
import '//resources/ash/common/cr_elements/cr_radio_group/cr_radio_group.js';
import '//resources/polymer/v3_0/iron-icon/iron-icon.js';
import '//resources/polymer/v3_0/iron-iconset-svg/iron-iconset-svg.js';
import '../../components/buttons/oobe_next_button.js';
import '../../components/buttons/oobe_text_button.js';
import '../../components/oobe_icons.html.js';
import '../../components/common_styles/cr_card_radio_group_styles.css.js';
import '../../components/common_styles/oobe_common_styles.css.js';
import '../../components/common_styles/oobe_dialog_host_styles.css.js';
import '../../components/dialogs/oobe_adaptive_dialog.js';

import {PolymerElementProperties} from '//resources/polymer/v3_0/polymer/interfaces.js';
import {mixinBehaviors, PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {LoginScreenBehavior, LoginScreenBehaviorInterface} from '../../components/behaviors/login_screen_behavior.js';
import {MultiStepBehavior, MultiStepBehaviorInterface} from '../../components/behaviors/multi_step_behavior.js';
import {OobeI18nMixin, OobeI18nMixinInterface} from '../../components/mixins/oobe_i18n_mixin.js';
import {OobeUiState} from '../../components/display_manager_types.js';

import {getTemplate} from './theme_selection.html.js';

const ThemeSelectionScreenElementBase =
    mixinBehaviors(
        [LoginScreenBehavior, MultiStepBehavior],
        OobeI18nMixin(PolymerElement)) as {
      new (): PolymerElement & OobeI18nMixinInterface &
          LoginScreenBehaviorInterface & MultiStepBehaviorInterface,
    };

interface ThemeSelectionScreenData {
  selectedTheme: string;
  shouldShowReturn: boolean;
}

/**
 * Enum to represent steps on the theme selection screen.
 * Currently there is only one step, but we still use
 * MultiStepBehavior because it provides implementation of
 * things like processing 'focus-on-show' class
 */
enum ThemeSelectionStep {
  OVERVIEW = 'overview',
}

/**
 * Available themes. The values should be in sync with the enum
 * defined in theme_selection_screen.h
 */
enum SelectedTheme {
  AUTO = 0,
  DARK = 1,
  LIGHT = 2,
}

/**
 * Available user actions.
 */
enum UserAction {
  SELECT = 'select',
  NEXT = 'next',
  RETURN = 'return',
}

class ThemeSelectionScreen extends ThemeSelectionScreenElementBase {
  static get is() {
    return 'theme-selection-element' as const;
  }

  static get template(): HTMLTemplateElement {
    return getTemplate();
  }

  static get properties(): PolymerElementProperties {
    return {
      /**
       * Indicates selected theme
       */
      selectedTheme: {type: String, value: 'auto', observer: 'onThemeChanged_'},

      /**
       * Indicates if the device is used in tablet mode
       */
      isInTabletMode_: {
        type: Boolean,
        value: false,
      },

      /**
       * Whether the button to return to CHOOBE screen should be shown.
       */
      shouldShowReturn_: {
        type: Boolean,
        value: false,
      },
    };
  }

  private selectedTheme: string;
  private isInTabletMode_: boolean;
  private shouldShowReturn_: boolean;

  override get UI_STEPS() {
    return ThemeSelectionStep;
  }

  // eslint-disable-next-line @typescript-eslint/naming-convention
  override defaultUIStep(): ThemeSelectionStep {
    return ThemeSelectionStep.OVERVIEW;
  }

  /**
   * Updates "device in tablet mode" state when tablet mode is changed.
   * Overridden from LoginScreenBehavior.
   * @param isInTabletMode True when in tablet mode.
   */
  override setTabletModeState(isInTabletMode: boolean) {
    this.isInTabletMode_ = isInTabletMode;
  }

  override ready(): void {
    super.ready();
    this.initializeLoginScreen('ThemeSelectionScreen');
  }

  /**
   * @param data Screen init payload.
   */
  private onBeforeShow(data: ThemeSelectionScreenData): void {
    this.selectedTheme = data.selectedTheme!;
    this.shouldShowReturn_ = data['shouldShowReturn'];
  }

  // eslint-disable-next-line @typescript-eslint/naming-convention
  override getOobeUIInitialState(): OobeUiState {
    return OobeUiState.THEME_SELECTION;
  }

  private onNextClicked_(): void {
    this.userActed(UserAction.NEXT);
  }

  private onThemeChanged_(themeSelect:string, oldTheme?: string): void {
    if (oldTheme === undefined) {
      return;
    }
    if (themeSelect === 'auto') {
      this.userActed([UserAction.SELECT, SelectedTheme.AUTO]);
    }
    if (themeSelect === 'light') {
      this.userActed([UserAction.SELECT, SelectedTheme.LIGHT]);
    }
    if (themeSelect === 'dark') {
      this.userActed([UserAction.SELECT, SelectedTheme.DARK]);
    }
  }

  private onReturnClicked_(): void {
    this.userActed(UserAction.RETURN);
  }
}

declare global {
  interface HTMLElementTagNameMap {
    [ThemeSelectionScreen.is]: ThemeSelectionScreen;
  }
}

customElements.define(ThemeSelectionScreen.is, ThemeSelectionScreen);
