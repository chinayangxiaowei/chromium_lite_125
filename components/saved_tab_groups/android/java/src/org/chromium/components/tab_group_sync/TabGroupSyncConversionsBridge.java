// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.tab_group_sync;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;

import org.chromium.url.GURL;

/**
 * Java counterpart to the C++ TabGroupSyncConversionsBridge class. This class has no public members
 * or methods and is meant as a private factory to build {@link SavedTabGroup} instances.
 */
@JNINamespace("tab_groups")
public class TabGroupSyncConversionsBridge {

    @CalledByNative
    private static SavedTabGroup createGroup(
            String syncId,
            int localId,
            String title,
            int color,
            long creationTimeMs,
            long updateTimeMs) {
        SavedTabGroup group = new SavedTabGroup();
        group.syncId = syncId;
        group.localId = localId == -1 ? null : localId;
        group.title = title;
        group.color = color;
        assert group.color != -1;
        group.creationTimeMs = creationTimeMs;
        group.updateTimeMs = updateTimeMs;
        return group;
    }

    @CalledByNative
    private static SavedTabGroupTab createTabAndMaybeAddToGroup(
            String syncId,
            int localId,
            String syncGroupId,
            int position,
            GURL url,
            String title,
            long creationTimeMs,
            long updateTimeMs,
            SavedTabGroup group) {
        SavedTabGroupTab tab = new SavedTabGroupTab();
        tab.syncId = syncId;
        tab.localId = localId == -1 ? null : localId;
        tab.syncGroupId = syncGroupId;
        tab.position = position == -1 ? null : position;
        tab.url = url;
        tab.title = title;
        tab.creationTimeMs = creationTimeMs;
        tab.updateTimeMs = updateTimeMs;
        if (group != null) {
            group.savedTabs.add(tab);
        }
        return tab;
    }
}
