// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tasks.tab_groups;

import org.chromium.base.Token;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.tab_groups.TabGroupColorId;

import java.util.List;

/** An interface to be notified about changes to a {@link TabGroupModelFilter}. */
public interface TabGroupModelFilterObserver {
    /**
     * This method is called before a tab is moved to form a group or moved into an existed group.
     *
     * @param movedTab The {@link Tab} which will be moved. If a group will be merged to a tab or
     *     another group, this is the last tab of the merged group.
     * @param newRootId The new root id of the group after merge.
     */
    default void willMergeTabToGroup(Tab movedTab, int newRootId) {}

    /**
     * This method is called before a group is moved.
     *
     * @param tabModelOldIndex The old index of the {@code movedTab} in the {@link TabModel}.
     * @param tabModelNewIndex The new index of the {@code movedTab} in the {@link TabModel}.
     */
    default void willMoveTabGroup(int tabModelOldIndex, int tabModelNewIndex) {}

    /**
     * This method is called before a tab within a group is moved out of the group.
     *
     * @param movedTab The tab which will be moved.
     * @param newRootId The new root id of the group from which {@code movedTab} is moved out.
     */
    default void willMoveTabOutOfGroup(Tab movedTab, int newRootId) {}

    /**
     * This method is called after a tab is moved to form a group or moved into an existed group.
     *
     * @param movedTab The {@link Tab} which has been moved. If a group is merged to a tab or
     *     another group, this is the last tab of the merged group.
     * @param selectedTabIdInGroup The id of the selected {@link Tab} in group.
     */
    default void didMergeTabToGroup(Tab movedTab, int selectedTabIdInGroup) {}

    /**
     * This method is called after a group is moved.
     *
     * @param movedTab The tab which has been moved. This is the last tab within the group.
     * @param tabModelOldIndex The old index of the {@code movedTab} in the {@link TabModel}.
     * @param tabModelNewIndex The new index of the {@code movedTab} in the {@link TabModel}.
     */
    default void didMoveTabGroup(Tab movedTab, int tabModelOldIndex, int tabModelNewIndex) {}

    /**
     * This method is called after a tab within a group is moved.
     *
     * @param movedTab The tab which has been moved.
     * @param tabModelOldIndex The old index of the {@code movedTab} in the {@link TabModel}.
     * @param tabModelNewIndex The new index of the {@code movedTab} in the {@link TabModel}.
     */
    default void didMoveWithinGroup(Tab movedTab, int tabModelOldIndex, int tabModelNewIndex) {}

    /**
     * This method is called after a tab within a group is moved out of the group.
     *
     * @param movedTab The tab which has been moved.
     * @param prevFilterIndex The index in {@link TabGroupModelFilter} of the group where {@code
     *     moveTab} is in before ungrouping.
     */
    default void didMoveTabOutOfGroup(Tab movedTab, int prevFilterIndex) {}

    /**
     * This method is called after a group is created manually by user. Either using the
     * TabListEditor (Group tab menu item) or using drag and drop.
     *
     * @param tabs The list of modified {@link Tab}s.
     * @param tabOriginalIndex The original tab index for each modified tab.
     * @param tabOriginalRootId The original root id for each modified tab.
     * @param tabOriginalTabGroupId The original tab group id for each modified tab.
     * @param destinationGroupTitle The original destination group title.
     * @param destinationGroupColorId The original destination group color id.
     */
    default void didCreateGroup(
            List<Tab> tabs,
            List<Integer> tabOriginalIndex,
            List<Integer> tabOriginalRootId,
            List<Token> tabOriginalTabGroupId,
            String destinationGroupTitle,
            int destinationGroupColorId) {}

    /**
     * This method is called after a new tab group is created, either through drag and drop, the tab
     * selection editor, or by longpressing a link on a tab and using the context menu.
     *
     * @param destinationTab The destination tab of the group after merge.
     * @param filter The {@link TabGroupModelFilter} that the new group event triggers on.
     */
    default void didCreateNewGroup(Tab destinationTab, TabGroupModelFilter filter) {}

    /**
     * This method is called after a new title is set on a tab group.
     *
     * @param rootId The current rootId of the tab group.
     * @param newTitle The new title.
     */
    default void didChangeTabGroupTitle(int rootId, String newTitle) {}

    /**
     * This method is called after a new color is set on a tab group.
     *
     * @param rootId The current rootId of the tab group.
     * @param newColor The new color.
     */
    default void didChangeTabGroupColor(int rootId, @TabGroupColorId int newColor) {}

    /**
     * When a tab group's root id needs to change because the tab whose id was previously being used
     * as the root ids is no longer part of the group. This could be a tab deletion that has not yet
     * been committed. Undo operations will not reverse this operation, as it does not have any user
     * facing effects.
     *
     * @param oldRootId The previous root id.
     * @param newRootId The new root id.
     */
    default void didChangeGroupRootId(int oldRootId, int newRootId) {}

    /**
     * Called when a tab group is removed. This could be the result of closing tabs inside the group
     * or by ungrouping tabs from the group.
     *
     * @param oldRootId The root id the group previous used.
     */
    default void didRemoveTabGroup(int oldRootId) {}
}
