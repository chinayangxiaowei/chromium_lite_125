// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.touch_to_fill.payments;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

/**
 * JNI wrapper for C++ TouchToFillPaymentMethodViewController. Delegates calls from Java to native.
 */
@JNINamespace("autofill")
class TouchToFillPaymentMethodControllerBridge
        implements TouchToFillPaymentMethodComponent.Delegate {
    private long mNativeTouchToFillPaymentMethodViewController;

    private TouchToFillPaymentMethodControllerBridge(
            long nativeTouchToFillPaymentMethodViewController) {
        mNativeTouchToFillPaymentMethodViewController =
                nativeTouchToFillPaymentMethodViewController;
    }

    @CalledByNative
    private static TouchToFillPaymentMethodControllerBridge create(
            long nativeTouchToFillPaymentMethodViewController) {
        return new TouchToFillPaymentMethodControllerBridge(
                nativeTouchToFillPaymentMethodViewController);
    }

    @CalledByNative
    private void onNativeDestroyed() {
        mNativeTouchToFillPaymentMethodViewController = 0;
    }

    @Override
    public void onDismissed(boolean dismissedByUser) {
        if (mNativeTouchToFillPaymentMethodViewController != 0) {
            TouchToFillPaymentMethodControllerBridgeJni.get()
                    .onDismissed(mNativeTouchToFillPaymentMethodViewController, dismissedByUser);
        }
    }

    @Override
    public void scanCreditCard() {
        if (mNativeTouchToFillPaymentMethodViewController != 0) {
            TouchToFillPaymentMethodControllerBridgeJni.get()
                    .scanCreditCard(mNativeTouchToFillPaymentMethodViewController);
        }
    }

    @Override
    public void showPaymentMethodSettings() {
        if (mNativeTouchToFillPaymentMethodViewController != 0) {
            TouchToFillPaymentMethodControllerBridgeJni.get()
                    .showPaymentMethodSettings(mNativeTouchToFillPaymentMethodViewController);
        }
    }

    @Override
    public void suggestionSelected(String uniqueId, boolean isVirtual) {
        if (mNativeTouchToFillPaymentMethodViewController != 0) {
            TouchToFillPaymentMethodControllerBridgeJni.get()
                    .suggestionSelected(
                            mNativeTouchToFillPaymentMethodViewController, uniqueId, isVirtual);
        }
    }

    @NativeMethods
    interface Natives {
        void onDismissed(
                long nativeTouchToFillPaymentMethodViewController, boolean dismissedByUser);

        void scanCreditCard(long nativeTouchToFillPaymentMethodViewController);

        void showPaymentMethodSettings(long nativeTouchToFillPaymentMethodViewController);

        void suggestionSelected(
                long nativeTouchToFillPaymentMethodViewController,
                String uniqueId,
                boolean isVirtual);
    }
}
