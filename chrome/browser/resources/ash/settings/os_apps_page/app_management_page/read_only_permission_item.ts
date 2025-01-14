// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/ash/common/cr_elements/policy/cr_policy_indicator.js';

import {I18nMixin} from 'chrome://resources/ash/common/cr_elements/i18n_mixin.js';
import {App} from 'chrome://resources/cr_components/app_management/app_management.mojom-webui.js';
import {PermissionTypeIndex} from 'chrome://resources/cr_components/app_management/permission_constants.js';
import {getPermission} from 'chrome://resources/cr_components/app_management/util.js';
import {assert} from 'chrome://resources/js/assert.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './read_only_permission_item.html.js';
import {getPermissionDescriptionString} from './util.js';

const AppManagementReadOnlyPermissionItemElementBase =
    I18nMixin(PolymerElement);

export class AppManagementReadOnlyPermissionItemElement extends
    AppManagementReadOnlyPermissionItemElementBase {
  static get is() {
    return 'app-management-read-only-permission-item' as const;
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /**
       * The name of the permission, to be displayed to the user.
       */
      permissionLabel: String,

      /**
       * A string version of the permission type. Must be a value of the
       * permission type enum in appManagement.mojom.PermissionType.
       */
      permissionType: {
        type: String,
        reflectToAttribute: true,
      },

      icon: String,

      app: Object,

      /**
       * True if the permission type is available for the app.
       */
      available_: {
        type: Boolean,
        computed: 'computeAvailable_(app, permissionType)',
        reflectToAttribute: true,
      },
    };
  }

  app: App;
  permissionLabel: string;
  permissionType: PermissionTypeIndex;
  icon: string;
  private available_: boolean;

  private computeAvailable_(
      app: App|undefined,
      permissionType: PermissionTypeIndex|undefined): boolean {
    if (app === undefined || permissionType === undefined) {
      return false;
    }

    return getPermission(app, permissionType) !== undefined;
  }

  private getPermissionDescriptionString_(
      app: App|undefined,
      permissionType: PermissionTypeIndex|undefined): string {
    return getPermissionDescriptionString(app, permissionType);
  }

  private isManaged_(
      app: App|undefined,
      permissionType: PermissionTypeIndex|undefined): boolean {
    if (app === undefined || permissionType === undefined) {
      return false;
    }

    const permission = getPermission(app, permissionType);
    assert(permission);

    return permission.isManaged;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    [AppManagementReadOnlyPermissionItemElement.is]:
        AppManagementReadOnlyPermissionItemElement;
  }
}

customElements.define(
    AppManagementReadOnlyPermissionItemElement.is,
    AppManagementReadOnlyPermissionItemElement);
