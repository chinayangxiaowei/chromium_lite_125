// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.theme;

import android.content.Context;
import android.content.res.ColorStateList;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.ObserverList;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.lifecycle.TopResumedActivityChangedObserver;
import org.chromium.chrome.browser.ui.desktop_windowing.AppHeaderUtils;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;

/** An abstract class that provides the current theme color. */
public abstract class ThemeColorProvider implements TopResumedActivityChangedObserver {
    /** An interface to be notified about changes to the theme color. */
    public interface ThemeColorObserver {
        /**
         * @param color The new color the observer should use.
         * @param shouldAnimate Whether the change of color should be animated.
         */
        void onThemeColorChanged(@ColorInt int color, boolean shouldAnimate);
    }

    /** An interface to be notified about changes to the tint. */
    public interface TintObserver {
        /**
         * @param tint The new tint the observer should use, without applying Activity state
         *     (focused vs unfocused) rules. This should be used for elements that don't adjust tint
         *     based on Activity focus.
         * @param activityFocusTint The tint the observer should use including consideration for
         *     whether the Activity is focused. This should be used for elements that do adjust tint
         *     based on Activity focus.
         * @param brandedColorScheme The {@link BrandedColorScheme} the observer should use.
         */
        void onTintChanged(
                ColorStateList tint,
                ColorStateList activityFocusTint,
                @BrandedColorScheme int brandedColorScheme);
    }

    /** Current primary color. */
    private @ColorInt int mPrimaryColor;

    /** The {@link BrandedColorScheme} for the current theme. */
    private @Nullable @BrandedColorScheme Integer mBrandedColorScheme;

    /**
     * The primary icon tint for the current theme, that does not take the activity focus state into
     * account.
     */
    private ColorStateList mTint;

    /** The icon tint for the current theme, that takes the activity focus state into account. */
    private ColorStateList mActivityFocusTint;

    /** List of {@link ThemeColorObserver}s. These are used to broadcast events to listeners. */
    private final ObserverList<ThemeColorObserver> mThemeColorObservers;

    /** List of {@link TintObserver}s. These are used to broadcast events to listeners. */
    private final ObserverList<TintObserver> mTintObservers;

    /**
     * The {@link ActivityLifecycleDispatcher} instance associated with the current activity, if
     * available.
     */
    @Nullable protected ActivityLifecycleDispatcher mActivityLifecycleDispatcher;

    /**
     * Whether the current activity is the top resumed activity. This is only relevant for use in
     * the desktop windowing mode, and is initially assumed to be true to enforce default
     * ThemeColorProvider behavior if activity lifecycle observation is not done in this class.
     */
    protected boolean mIsTopResumedActivity;

    /** A boolean supplier to determine whether the current activity is in a desktop window. */
    protected ObservableSupplier<Boolean> mDesktopWindowModeSupplier;

    /**
     * @param context The {@link Context} that is used to retrieve color related resources.
     * @param activityLifecycleDispatcher The {@link ActivityLifecycleDispatcher} instance
     *     associated with the current activity. {@code null} if activity lifecycle observation is
     *     not required.
     */
    public ThemeColorProvider(
            Context context, @Nullable ActivityLifecycleDispatcher activityLifecycleDispatcher) {
        mThemeColorObservers = new ObserverList<>();
        mTintObservers = new ObserverList<>();
        mTint = ThemeUtils.getThemedToolbarIconTint(context, BrandedColorScheme.APP_DEFAULT);

        // Activity lifecycle observation for activity focus change.
        if (activityLifecycleDispatcher != null) {
            mActivityLifecycleDispatcher = activityLifecycleDispatcher;
            mActivityLifecycleDispatcher.register(this);
        }
        mIsTopResumedActivity =
                AppHeaderUtils.isActivityFocusedAtStartup(mActivityLifecycleDispatcher);

        mActivityFocusTint =
                ThemeUtils.getThemedToolbarIconTintForActivityState(
                        context, BrandedColorScheme.APP_DEFAULT, mIsTopResumedActivity);
    }

    @Override
    public void onTopResumedActivityChanged(boolean isTopResumedActivity) {
        // TODO (crbug/328055199): Check if losing focus to a non-Chrome task.
        if (!AppHeaderUtils.isAppInDesktopWindow(mDesktopWindowModeSupplier)) return;
        mIsTopResumedActivity = isTopResumedActivity;
    }

    /**
     * Sets an {@link ObservableSupplier} to observe desktop windowing mode changes.
     *
     * @param desktopWindowModeSupplier The {@link ObservableSupplier} to observe desktop windowing
     *     mode changes.
     */
    public void setDesktopWindowModeSupplier(
            ObservableSupplier<Boolean> desktopWindowModeSupplier) {
        mDesktopWindowModeSupplier = desktopWindowModeSupplier;
    }

    /**
     * @param observer Adds a {@link ThemeColorObserver} that will be notified when the theme color
     *     changes. This method does not trigger the observer.
     */
    public void addThemeColorObserver(ThemeColorObserver observer) {
        mThemeColorObservers.addObserver(observer);
    }

    /**
     * @param observer Removes the observer so it no longer receives theme color changes.
     */
    public void removeThemeColorObserver(ThemeColorObserver observer) {
        mThemeColorObservers.removeObserver(observer);
    }

    /**
     * @param observer Adds a {@link TintObserver} that will be notified when the tint changes. This
     *                 method does not trigger the observer.
     */
    public void addTintObserver(TintObserver observer) {
        mTintObservers.addObserver(observer);
    }

    /**
     * @param observer Removes the observer so it no longer receives tint changes.
     */
    public void removeTintObserver(TintObserver observer) {
        mTintObservers.removeObserver(observer);
    }

    /**
     * @return The current theme color of this provider.
     */
    public @ColorInt int getThemeColor() {
        return mPrimaryColor;
    }

    /**
     * @return The current tint of this provider, that does not take the activity focus state into
     *     account.
     */
    public ColorStateList getTint() {
        return mTint;
    }

    /**
     * @return The current {@link BrandedColorScheme} of this provider.
     */
    public @BrandedColorScheme int getBrandedColorScheme() {
        return mBrandedColorScheme != null ? mBrandedColorScheme : BrandedColorScheme.APP_DEFAULT;
    }

    /** Clears out the observer lists. */
    public void destroy() {
        mThemeColorObservers.clear();
        mTintObservers.clear();
        if (mActivityLifecycleDispatcher != null) {
            mActivityLifecycleDispatcher.unregister(this);
        }
    }

    protected void updatePrimaryColor(@ColorInt int color, boolean shouldAnimate) {
        if (mPrimaryColor == color) return;
        mPrimaryColor = color;
        for (ThemeColorObserver observer : mThemeColorObservers) {
            observer.onThemeColorChanged(color, shouldAnimate);
        }
    }

    protected void updateTint(
            @NonNull ColorStateList tint,
            @NonNull ColorStateList activityFocusTint,
            @BrandedColorScheme int brandedColorScheme) {
        if (tint == mTint && activityFocusTint == mActivityFocusTint) return;
        mTint = tint;
        mActivityFocusTint = activityFocusTint;
        mBrandedColorScheme = brandedColorScheme;

        for (TintObserver observer : mTintObservers) {
            observer.onTintChanged(tint, activityFocusTint, brandedColorScheme);
        }
    }

    protected ColorStateList calculateActivityFocusTint(
            Context context, @BrandedColorScheme int brandedColorScheme) {
        var iconTint = ThemeUtils.getThemedToolbarIconTint(context, brandedColorScheme);
        return mActivityLifecycleDispatcher == null
                        || (!AppHeaderUtils.isAppInDesktopWindow(mDesktopWindowModeSupplier))
                ? iconTint
                : ThemeUtils.getThemedToolbarIconTintForActivityState(
                        context, brandedColorScheme, mIsTopResumedActivity);
    }
}
