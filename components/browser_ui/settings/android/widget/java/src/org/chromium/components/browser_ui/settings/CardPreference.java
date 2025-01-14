// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.browser_ui.settings;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.method.LinkMovementMethod;
import android.util.AttributeSet;
import android.view.View.OnClickListener;

import androidx.preference.PreferenceViewHolder;

import org.chromium.ui.widget.ChromeImageView;
import org.chromium.ui.widget.TextViewWithClickableSpans;

/**
 * A preference wrapper for {@link MaterialCardViewNoShadow} with an icon, a text message and an
 * optional close button.
 */
public class CardPreference extends TextMessagePreference {
    private CharSequence mSummary;
    private Drawable mIconDrawable;
    private int mCloseIconVisibility;
    private OnClickListener mOnCloseClickListener;

    private TextViewWithClickableSpans mDescriptionView;
    private ChromeImageView mIcon;
    private ChromeImageView mCloseIcon;

    /** Constructor for inflating from XML. */
    public CardPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.card_preference);
        setSelectable(false);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        mDescriptionView = (TextViewWithClickableSpans) holder.findViewById(R.id.summary);
        mIcon = (ChromeImageView) holder.findViewById(R.id.icon);
        mCloseIcon = (ChromeImageView) holder.findViewById(R.id.close_icon);

        mDescriptionView.setText(mSummary);
        mDescriptionView.setMovementMethod(LinkMovementMethod.getInstance());
        mIcon.setImageDrawable(mIconDrawable);
        mCloseIcon.setVisibility(mCloseIconVisibility);
        mCloseIcon.setOnClickListener(mOnCloseClickListener);
    }

    /**
     * Set card summary.
     *
     * @param summary Summary char sequence.
     */
    @Override
    public void setSummary(CharSequence summary) {
        mSummary = summary;
    }

    /**
     * Set card icon drawable.
     *
     * @param iconDrawable Drawable to be shown.
     */
    public void setIconDrawable(Drawable iconDrawable) {
        mIconDrawable = iconDrawable;
    }

    /**
     * Set close icon visibility.
     *
     * @param visibility Close icon visibility.
     */
    public void setCloseIconVisibility(int visibility) {
        mCloseIconVisibility = visibility;
    }

    /**
     * Set on close click listener.
     *
     * @param onCloseClickListener The close icon click listener.
     */
    public void setOnCloseClickListener(OnClickListener onCloseClickListener) {
        this.mOnCloseClickListener = onCloseClickListener;
    }
}
