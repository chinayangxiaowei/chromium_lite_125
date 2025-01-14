// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync;

import org.jni_zero.NativeMethods;

import org.chromium.base.FeatureMap;

/** Java accessor for base/android/feature_map.h state. */
public final class SyncFeatureMap extends FeatureMap {
    public static final String ENABLE_BOOKMARK_FOLDERS_FOR_ACCOUNT_STORAGE =
            "EnableBookmarkFoldersForAccountStorage";

    private static final SyncFeatureMap sInstance = new SyncFeatureMap();

    // Do not instantiate this class.
    private SyncFeatureMap() {}

    /**
     * @return the singleton SyncFeatureMap.
     */
    public static SyncFeatureMap getInstance() {
        return sInstance;
    }

    /** Convenience method to call {@link #isEnabledInNative(String)} statically. */
    public static boolean isEnabled(String featureName) {
        return getInstance().isEnabledInNative(featureName);
    }

    @Override
    protected long getNativeMap() {
        return SyncFeatureMapJni.get().getNativeMap();
    }

    @NativeMethods
    public interface Natives {
        long getNativeMap();
    }
}
