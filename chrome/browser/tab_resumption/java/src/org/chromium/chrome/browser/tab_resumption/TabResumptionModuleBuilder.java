// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab_resumption;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import androidx.annotation.NonNull;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.magic_stack.ModuleConfigChecker;
import org.chromium.chrome.browser.magic_stack.ModuleDelegate;
import org.chromium.chrome.browser.magic_stack.ModuleProvider;
import org.chromium.chrome.browser.magic_stack.ModuleProviderBuilder;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab_resumption.TabResumptionModuleMetricsUtils.ModuleNotShownReason;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider;
import org.chromium.chrome.browser.tab_ui.ThumbnailProvider;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;

public class TabResumptionModuleBuilder implements ModuleProviderBuilder, ModuleConfigChecker {
    private final Context mContext;
    private final ObservableSupplier<Profile> mProfileSupplier;
    private final ObservableSupplier<TabContentManager> mTabContentManagerSupplier;

    // Foreign Session data source that listens to login / sync status changes. Shared among data
    // providers to reduce resource use, and ref-counted to ensure proper resource management.
    private ForeignSessionTabResumptionDataSource mForeignSessionTabResumptionDataSource;
    private int mForeignSessionTabResumptionDataSourceRefCount;
    private TabListFaviconProvider mFaviconProvider;

    public TabResumptionModuleBuilder(
            @NonNull Context context,
            @NonNull ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<TabContentManager> tabContentManagerSupplier) {
        mContext = context;
        mProfileSupplier = profileSupplier;
        mTabContentManagerSupplier = tabContentManagerSupplier;
    }

    /** Build {@link ModuleProvider} for the tab resumption module. */
    @Override
    public boolean build(
            @NonNull ModuleDelegate moduleDelegate,
            @NonNull Callback<ModuleProvider> onModuleBuiltCallback) {
        Profile profile = getRegularProfile();

        Integer notShownReason =
                TabResumptionModuleEnablement.computeModuleNotShownReason(
                        moduleDelegate, getRegularProfile());
        if (notShownReason != null) {
            TabResumptionModuleMetricsUtils.recordModuleNotShownReason(notShownReason.intValue());
            return false;
        }

        // TODO(b/332588018): Conditionally instantiate TabResumptionDataProvider on this.
        assert TabResumptionModuleEnablement.ForeignSession.shouldMakeProvider(profile);

        addRefToDataSource();
        TabResumptionDataProvider dataProvider =
                new ForeignSessionTabResumptionDataProvider(
                        mForeignSessionTabResumptionDataSource, this::removeRefToDataSource);
        // TODO(b/332588018): Uses TabListFaviconProvider to replace UrlImageProvider.
        UrlImageProvider urlImageProvider = new UrlImageProvider(profile, mContext);
        if (mFaviconProvider == null) {
            mFaviconProvider =
                    new TabListFaviconProvider(
                            mContext,
                            false,
                            org.chromium.chrome.browser.tab_ui.R.dimen
                                    .favicon_corner_radius_polished);
            mFaviconProvider.initWithNative(profile);
        }

        assert mTabContentManagerSupplier.hasValue();
        TabResumptionModuleCoordinator coordinator =
                new TabResumptionModuleCoordinator(
                        mContext,
                        moduleDelegate,
                        dataProvider,
                        urlImageProvider,
                        mFaviconProvider,
                        getThumbnailProvider(mTabContentManagerSupplier.get()));
        onModuleBuiltCallback.onResult(coordinator);
        return true;
    }

    /** Create view for the tab resumption module. */
    @Override
    public ViewGroup createView(@NonNull ViewGroup parentView) {
        return (ViewGroup)
                LayoutInflater.from(mContext)
                        .inflate(R.layout.tab_resumption_module_layout, parentView, false);
    }

    /** Bind the property model for the tab resumption module. */
    @Override
    public void bind(
            @NonNull PropertyModel model,
            @NonNull ViewGroup view,
            @NonNull PropertyKey propertyKey) {
        TabResumptionModuleViewBinder.bind(model, view, propertyKey);
    }

    // ModuleEligibilityChecker implementation:

    @Override
    public boolean isEligible() {
        // This function may be called by MainSettings when a profile hasn't been initialized yet.
        // See b/324138242.
        if (!mProfileSupplier.hasValue()) return false;

        if (TabResumptionModuleEnablement.isFeatureEnabled()) return true;

        TabResumptionModuleMetricsUtils.recordModuleNotShownReason(
                ModuleNotShownReason.FEATURE_DISABLED);
        return false;
    }

    /** Gets the regular profile if exists. */
    private Profile getRegularProfile() {
        assert mProfileSupplier.hasValue();

        Profile profile = mProfileSupplier.get();
        // It is possible that an incognito profile is provided by the supplier. See b/326619334.
        return profile.isOffTheRecord() ? profile.getOriginalProfile() : profile;
    }

    private void addRefToDataSource() {
        if (mForeignSessionTabResumptionDataSourceRefCount == 0) {
            assert mForeignSessionTabResumptionDataSource == null;
            Profile profile = getRegularProfile();
            mForeignSessionTabResumptionDataSource =
                    ForeignSessionTabResumptionDataSource.createFromProfile(profile);
        }
        ++mForeignSessionTabResumptionDataSourceRefCount;
    }

    private void removeRefToDataSource() {
        assert mForeignSessionTabResumptionDataSource != null;
        --mForeignSessionTabResumptionDataSourceRefCount;
        if (mForeignSessionTabResumptionDataSourceRefCount == 0) {
            mForeignSessionTabResumptionDataSource.destroy();
            mForeignSessionTabResumptionDataSource = null;
        }
    }

    static ThumbnailProvider getThumbnailProvider(TabContentManager tabContentManager) {
        return (tabId, thumbnailSize, callback, forceUpdate, writeBack, isSelected) -> {
            tabContentManager.getTabThumbnailWithCallback(
                    tabId, thumbnailSize, callback, forceUpdate, writeBack);
        };
    }
}
