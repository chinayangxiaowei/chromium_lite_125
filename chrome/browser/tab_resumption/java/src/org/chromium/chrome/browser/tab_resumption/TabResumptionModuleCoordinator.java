// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab_resumption;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.chrome.browser.magic_stack.ModuleDelegate;
import org.chromium.chrome.browser.magic_stack.ModuleProvider;
import org.chromium.chrome.browser.tab_resumption.TabResumptionModuleUtils.SuggestionClickCallbacks;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider;
import org.chromium.chrome.browser.tab_ui.ThumbnailProvider;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

/**
 * The Coordinator for the tab resumption module, which can be embedded by surfaces like NTP or
 * Start surface.
 */
public class TabResumptionModuleCoordinator implements ModuleProvider {
    protected final Context mContext;
    protected final ModuleDelegate mModuleDelegate;
    protected final TabResumptionDataProvider mDataProvider;
    protected final UrlImageProvider mUrlImageProvider;
    protected final PropertyModel mModel;
    protected final TabResumptionModuleMediator mMediator;

    public TabResumptionModuleCoordinator(
            @NonNull Context context,
            @NonNull ModuleDelegate moduleDelegate,
            @NonNull TabResumptionDataProvider dataProvider,
            @NonNull UrlImageProvider urlImageProvider,
            @NonNull TabListFaviconProvider faviconProvider,
            @NonNull ThumbnailProvider thumbnailProvider) {
        mContext = context;
        mModuleDelegate = moduleDelegate;
        mDataProvider = dataProvider;
        mUrlImageProvider = urlImageProvider;
        mModel = new PropertyModel(TabResumptionModuleProperties.ALL_KEYS);
        SuggestionClickCallbacks wrappedClickCallbacks =
                new SuggestionClickCallbacks() {
                    @Override
                    public void onSuggestionClickByUrl(GURL gurl) {
                        mModuleDelegate.onUrlClicked(gurl, getModuleType());
                    }

                    @Override
                    public void onSuggestionClickByTabId(int tabId) {
                        moduleDelegate.onTabClicked(tabId, getModuleType());
                    }
                };
        mMediator =
                new TabResumptionModuleMediator(
                        mContext,
                        mModuleDelegate,
                        mModel,
                        mDataProvider,
                        mUrlImageProvider,
                        faviconProvider,
                        thumbnailProvider,
                        wrappedClickCallbacks);
        mDataProvider.setStatusChangedCallback(this::showModule);
    }

    public void destroy() {
        mDataProvider.setStatusChangedCallback(null);
        mMediator.destroy();
        mUrlImageProvider.destroy();
        mDataProvider.destroy();
    }

    /** Show tab resumption module. */
    @Override
    public void showModule() {
        mMediator.loadModule();
    }

    @Override
    public int getModuleType() {
        return mMediator.getModuleType();
    }

    @Override
    public void hideModule() {
        destroy();
    }

    @Override
    public String getModuleContextMenuHideText(Context context) {
        return mMediator.getModuleContextMenuHideText(context);
    }

    @Override
    public void onContextMenuCreated() {}
}
