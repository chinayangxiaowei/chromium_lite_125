// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.tab_group_sync;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.url.GURL;

/**
 * This class is the Java counterpart to the C++ SavedTabGroupTab
 * (components/saved_tab_groups/saved_tab_group_tab.h) class.
 */
public class SavedTabGroupTab {
    /** The ID used to represent the tab in sync. */
    // TODO(shaktisahu): Decide if this will be used from Java to native flow. If yes, this ID
    //  can be nullable as well.
    public String syncId;

    /**
     * The ID representing the tab locally, as returned by {@link Tab#getId()}. It can be null, if
     * the {@link SavedTabGroupTab} represents a tab that doesn't exist locally yet.
     */
    public @Nullable Integer localId;

    /** The ID used to represent the tab's group in sync. */
    public @NonNull String syncGroupId;

    /** The title of the website this url is associated with. */
    public @Nullable String title;

    /**
     * The current position of the tab in relation to all other tabs in the group. A value of null
     * means that the group was not assigned a position and will be assigned one when it is added
     * into its saved group.
     */
    public @Nullable Integer position;

    /** The URL of the tab. */
    public @Nullable GURL url;

    /** Timestamp for when the tab was created. */
    public long creationTimeMs;

    /** Timestamp for when the tab was last updated. */
    public long updateTimeMs;
}
