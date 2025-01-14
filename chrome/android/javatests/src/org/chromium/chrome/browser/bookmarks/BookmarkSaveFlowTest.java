// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;

import android.widget.CompoundButton;

import androidx.test.espresso.Espresso;
import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.Callback;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.commerce.PriceTrackingUtils;
import org.chromium.chrome.browser.commerce.PriceTrackingUtilsJni;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksShim;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.sync.SyncTestRule;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.R;
import org.chromium.chrome.test.util.BookmarkTestUtil;
import org.chromium.chrome.test.util.ChromeRenderTestRule;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetTestSupport;
import org.chromium.components.commerce.core.CommerceSubscription;
import org.chromium.components.commerce.core.ShoppingService;
import org.chromium.components.power_bookmarks.PowerBookmarkMeta;
import org.chromium.components.power_bookmarks.ShoppingSpecifics;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.sync.SyncFeatureMap;
import org.chromium.content_public.browser.test.util.ClickUtils;
import org.chromium.content_public.browser.test.util.TestThreadUtils;
import org.chromium.url.GURL;

import java.io.IOException;
import java.util.concurrent.ExecutionException;

/** Tests for the bookmark save flow. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableFeatures({
    ChromeFeatureList.ANDROID_IMPROVED_BOOKMARKS,
    SyncFeatureMap.ENABLE_BOOKMARK_FOLDERS_FOR_ACCOUNT_STORAGE
})
// TODO(crbug.com/1168590): Once SyncTestRule supports batching, investigate batching this suite.
@DoNotBatch(reason = "SyncTestRule doesn't support batching.")
public class BookmarkSaveFlowTest {
    @Rule public final SyncTestRule mSyncTestRule = new SyncTestRule();

    @Rule
    public final ChromeRenderTestRule mRenderTestRule =
            ChromeRenderTestRule.Builder.withPublicCorpus()
                    .setBugComponent(ChromeRenderTestRule.Component.UI_BROWSER_BOOKMARKS)
                    .build();

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Rule public final JniMocker mJniMocker = new JniMocker();

    @Mock private ShoppingService mShoppingService;
    @Mock private PriceTrackingUtils.Natives mMockPriceTrackingUtilsJni;
    @Mock private UserEducationHelper mUserEducationHelper;
    @Mock private IdentityManager mIdentityManager;

    private ChromeTabbedActivity mActivity;
    private BookmarkSaveFlowCoordinator mBookmarkSaveFlowCoordinator;
    private BottomSheetController mBottomSheetController;
    private BottomSheetTestSupport mBottomSheetTestSupport;
    private BookmarkModel mBookmarkModel;
    private CoreAccountInfo mAccountInfo =
            CoreAccountInfo.createFromEmailAndGaiaId("test@gmail.com", "testGaiaId");

    @Before
    public void setUp() throws ExecutionException {
        mSyncTestRule.setUpAccountAndSignInForTesting();
        mActivity = mSyncTestRule.getActivity();

        // Setup mocks.
        mJniMocker.mock(PriceTrackingUtilsJni.TEST_HOOKS, mMockPriceTrackingUtilsJni);
        doReturn(mAccountInfo).when(mIdentityManager).getPrimaryAccountInfo(anyInt());

        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mBottomSheetController =
                            mActivity.getRootUiCoordinatorForTesting().getBottomSheetController();
                    mBottomSheetTestSupport = new BottomSheetTestSupport(mBottomSheetController);
                    mBookmarkModel = mActivity.getBookmarkModelForTesting();
                    mBookmarkSaveFlowCoordinator =
                            new BookmarkSaveFlowCoordinator(
                                    mActivity,
                                    mBottomSheetController,
                                    mShoppingService,
                                    mUserEducationHelper,
                                    ProfileManager.getLastUsedRegularProfile(),
                                    mIdentityManager);
                });

        loadBookmarkModel();
        doAnswer(
                        (invocation) -> {
                            ((Callback<Boolean>) invocation.getArgument(3)).onResult(true);
                            return null;
                        })
                .when(mMockPriceTrackingUtilsJni)
                .setPriceTrackingStateForBookmark(
                        any(Profile.class), anyLong(), anyBoolean(), any(), anyBoolean());
        doAnswer(
                        (invocation) -> {
                            ((Callback<Boolean>) invocation.getArgument(1)).onResult(true);
                            return null;
                        })
                .when(mShoppingService)
                .subscribe(any(CommerceSubscription.class), any());
        doAnswer(
                        (invocation) -> {
                            ((Callback<Boolean>) invocation.getArgument(1)).onResult(true);
                            return null;
                        })
                .when(mShoppingService)
                .unsubscribe(any(CommerceSubscription.class), any());
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testBookmarkSaveFlow() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(id);
                    return null;
                });
        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(), "bookmark_save_flow");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    @EnableFeatures(ChromeFeatureList.ANDROID_IMPROVED_BOOKMARKS)
    public void testBookmarkSaveFlow_improvedBookmarks() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(id);
                    return null;
                });
        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(), "bookmark_save_flow_improved");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    @EnableFeatures({
        ChromeFeatureList.ANDROID_IMPROVED_BOOKMARKS,
        ChromeFeatureList.REPLACE_SYNC_PROMOS_WITH_SIGN_IN_PROMOS,
        SyncFeatureMap.ENABLE_BOOKMARK_FOLDERS_FOR_ACCOUNT_STORAGE
    })
    public void testBookmarkSaveFlow_improvedBookmarks_accountBookmarksEnabled()
            throws IOException {
        CriteriaHelper.pollUiThread(() -> mBookmarkModel.getAccountMobileFolderId() != null);
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(id);
                    return null;
                });
        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(),
                "bookmark_save_flow_improved_account");
    }

    @Test
    @MediumTest
    public void testBookmarkSaveFlow_DestroyAfterHidden() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(id);
                    mBookmarkSaveFlowCoordinator.close();
                    return null;
                });
        CriteriaHelper.pollUiThread(
                () -> mBookmarkSaveFlowCoordinator.getIsDestroyedForTesting(),
                "Save flow coordinator not destroyed.");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testBookmarkSaveFlow_BookmarkMoved() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromExplicitTrackUi= */ false,
                            /* wasBookmarkMoved= */ true,
                            /* isNewBookmark= */ false);
                    return null;
                });
        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(),
                "bookmark_save_flow_bookmark_moved");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testBookmarkSaveFlow_WithShoppingListItem() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    PowerBookmarkMeta.Builder meta =
                            PowerBookmarkMeta.newBuilder()
                                    .setShoppingSpecifics(
                                            ShoppingSpecifics.newBuilder()
                                                    .setProductClusterId(1234L)
                                                    .build());
                    mBookmarkModel.setPowerBookmarkMeta(id, meta.build());
                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromHeuristicEntryPoint= */ false,
                            /* wasBookmarkMoved= */ false,
                            /* isNewBookmark= */ true,
                            meta.build());
                    return null;
                });
        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(),
                "bookmark_save_flow_shopping_list_item");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testBookmarkSaveFlow_WithShoppingListItem_fromHeuristicEntryPoint()
            throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    PowerBookmarkMeta.Builder meta =
                            PowerBookmarkMeta.newBuilder()
                                    .setShoppingSpecifics(
                                            ShoppingSpecifics.newBuilder()
                                                    .setProductClusterId(1234L)
                                                    .build());
                    mBookmarkModel.setPowerBookmarkMeta(id, meta.build());

                    doAnswer(
                                    args -> {
                                        ((Callback<Boolean>) args.getArgument(2)).onResult(true);
                                        return null;
                                    })
                            .when(mMockPriceTrackingUtilsJni)
                            .isBookmarkPriceTracked(any(), anyLong(), any());

                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromHeuristicEntryPoint= */ true,
                            /* wasBookmarkMoved= */ false,
                            /* isNewBookmark= */ false,
                            meta.build());
                    return null;
                });

        CriteriaHelper.pollUiThread(
                () -> {
                    CompoundButton toggle =
                            mBookmarkSaveFlowCoordinator
                                    .getViewForTesting()
                                    .findViewById(R.id.notification_switch);
                    return toggle.isChecked();
                });

        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(),
                "bookmark_save_flow_shopping_list_item_from_heuristic");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testBookmarkSaveFlow_WithShoppingListItem_fromHeuristicEntryPoint_saveFailed()
            throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    PowerBookmarkMeta.Builder meta =
                            PowerBookmarkMeta.newBuilder()
                                    .setShoppingSpecifics(
                                            ShoppingSpecifics.newBuilder()
                                                    .setProductClusterId(1234L));
                    mBookmarkModel.setPowerBookmarkMeta(id, meta.build());

                    doAnswer(
                                    args -> {
                                        ((Callback<Boolean>) args.getArgument(2)).onResult(false);
                                        return null;
                                    })
                            .when(mMockPriceTrackingUtilsJni)
                            .isBookmarkPriceTracked(any(), anyLong(), any());

                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromHeuristicEntryPoint= */ false,
                            /* wasBookmarkMoved= */ false,
                            /* isNewBookmark= */ false,
                            meta.build());
                    return null;
                });

        doAnswer(
                        (invocation) -> {
                            ((Callback<Boolean>) invocation.getArgument(3)).onResult(false);
                            return null;
                        })
                .when(mMockPriceTrackingUtilsJni)
                .setPriceTrackingStateForBookmark(
                        any(Profile.class), anyLong(), anyBoolean(), any(), anyBoolean());
        doAnswer(
                        (invocation) -> {
                            ((Callback<Boolean>) invocation.getArgument(1)).onResult(false);
                            return null;
                        })
                .when(mShoppingService)
                .subscribe(any(CommerceSubscription.class), any());
        onView(withId(R.id.notification_switch)).perform(click());

        CriteriaHelper.pollUiThread(
                () -> {
                    CompoundButton toggle =
                            mBookmarkSaveFlowCoordinator
                                    .getViewForTesting()
                                    .findViewById(R.id.notification_switch);
                    return !toggle.isChecked();
                });

        mRenderTestRule.render(
                mBookmarkSaveFlowCoordinator.getViewForTesting(),
                "bookmark_save_flow_shopping_list_item_from_heuristic_save_failed");
    }

    @Test
    @MediumTest
    public void testBookmarkSaveFlowEdit() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromHeuristicEntryPoint= */ false,
                            /* wasBookmarkMoved= */ false,
                            /* isNewBookmark= */ true);
                    return null;
                });
        ClickUtils.clickButton(mActivity.findViewById(R.id.bookmark_edit));
        onView(withText(mActivity.getResources().getString(R.string.edit_bookmark)))
                .check(matches(isDisplayed()));

        // Dismiss the activity.
        Espresso.pressBack();
    }

    @Test
    @MediumTest
    public void testBookmarkSaveFlowChooseFolder() throws IOException {
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> {
                    BookmarkId id = addBookmark("Test bookmark", new GURL("http://a.com"));
                    mBookmarkSaveFlowCoordinator.show(
                            id,
                            /* fromHeuristicEntryPoint= */ false,
                            /* wasBookmarkMoved= */ false,
                            /* isNewBookmark= */ true);
                    return null;
                });
        ClickUtils.clickButton(mActivity.findViewById(R.id.bookmark_select_folder));
        onView(withText(mActivity.getResources().getString(R.string.bookmark_choose_folder)))
                .check(matches(isDisplayed()));

        // Dismiss the activity.
        Espresso.pressBack();
    }

    private void loadBookmarkModel() {
        // Do not read partner bookmarks in setUp(), so that the lazy reading is covered.
        TestThreadUtils.runOnUiThreadBlocking(() -> PartnerBookmarksShim.kickOffReading(mActivity));
        BookmarkTestUtil.waitForBookmarkModelLoaded();
    }

    private BookmarkId addBookmark(final String title, final GURL url) throws ExecutionException {
        return TestThreadUtils.runOnUiThreadBlocking(
                () ->
                        mBookmarkModel.addBookmark(
                                mBookmarkModel.getDefaultBookmarkFolder(), 0, title, url));
    }
}
