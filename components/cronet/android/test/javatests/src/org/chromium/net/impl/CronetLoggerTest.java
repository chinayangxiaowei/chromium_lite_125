// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.impl;

import static android.os.Process.THREAD_PRIORITY_BACKGROUND;
import static android.os.Process.THREAD_PRIORITY_DEFAULT;

import static com.google.common.truth.Truth.assertThat;

import static org.chromium.net.CronetEngine.Builder.HTTP_CACHE_DISK_NO_HTTP;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.ConditionVariable;

import androidx.test.filters.SmallTest;

import com.google.protobuf.ByteString;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.chromium.base.test.util.DoNotBatch;
import org.chromium.net.CronetEngine;
import org.chromium.net.CronetLoggerTestRule;
import org.chromium.net.CronetTestRule;
import org.chromium.net.CronetTestRule.CronetImplementation;
import org.chromium.net.CronetTestRule.IgnoreFor;
import org.chromium.net.CronetTestRule.RequiresMinAndroidApi;
import org.chromium.net.ExperimentalCronetEngine;
import org.chromium.net.NativeTestServer;
import org.chromium.net.TestUrlRequestCallback;
import org.chromium.net.UrlRequest;
import org.chromium.net.httpflags.FlagValue;
import org.chromium.net.httpflags.Flags;
import org.chromium.net.impl.CronetEngineBuilderImpl.HttpCacheMode;
import org.chromium.net.impl.CronetLogger.CronetEngineBuilderInfo;
import org.chromium.net.impl.CronetLogger.CronetSource;
import org.chromium.net.impl.CronetLogger.CronetTrafficInfo;
import org.chromium.net.impl.CronetLogger.CronetVersion;

import java.time.Duration;
import java.util.AbstractMap;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

/** Test logging functionalities. */
@DoNotBatch(reason = "Some logging is done from one-time static initialization")
@RunWith(JUnit4.class)
@RequiresMinAndroidApi(Build.VERSION_CODES.O)
@IgnoreFor(
        implementations = {CronetImplementation.FALLBACK, CronetImplementation.AOSP_PLATFORM},
        reason = "CronetLoggerTestRule is supported only by the native implementation.")
public final class CronetLoggerTest {
    private final CronetTestRule mTestRule = CronetTestRule.withManualEngineStartup();
    private final CronetLoggerTestRule mLoggerTestRule = new CronetLoggerTestRule(TestLogger.class);

    @Rule public final RuleChain chain = RuleChain.outerRule(mTestRule).around(mLoggerTestRule);

    private TestLogger mTestLogger;
    private Context mContext;

    @Before
    public void setUp() {
        mContext = mTestRule.getTestFramework().getContext();
        mTestLogger = (TestLogger) mLoggerTestRule.mTestLogger;
        assertThat(NativeTestServer.startNativeTestServer(mContext)).isTrue();
    }

    @After
    public void tearDown() {
        mTestLogger = null;
        NativeTestServer.shutdownNativeTestServer();
    }

    @Test
    @SmallTest
    public void testCronetEngineInfoCreation() {
        CronetEngineBuilderImpl builder = new NativeCronetEngineBuilderImpl(mContext);
        CronetEngineBuilderInfo builderInfo = builder.toLoggerInfo();
        assertThat(builderInfo.isPublicKeyPinningBypassForLocalTrustAnchorsEnabled())
                .isEqualTo(builder.publicKeyPinningBypassForLocalTrustAnchorsEnabled());
        assertThat(builderInfo.getUserAgent()).isEqualTo(builder.getUserAgent());
        assertThat(builderInfo.getStoragePath()).isEqualTo(builder.storagePath());
        assertThat(builderInfo.isQuicEnabled()).isEqualTo(builder.quicEnabled());
        assertThat(builderInfo.isHttp2Enabled()).isEqualTo(builder.http2Enabled());
        assertThat(builderInfo.isBrotliEnabled()).isEqualTo(builder.brotliEnabled());
        assertThat(builderInfo.getHttpCacheMode()).isEqualTo(builder.publicBuilderHttpCacheMode());
        assertThat(builderInfo.getExperimentalOptions()).isEqualTo(builder.experimentalOptions());
        assertThat(builderInfo.isNetworkQualityEstimatorEnabled())
                .isEqualTo(builder.networkQualityEstimatorEnabled());
        assertThat(builderInfo.getThreadPriority())
                .isEqualTo(builder.threadPriority(THREAD_PRIORITY_BACKGROUND));
    }

    @Test
    @SmallTest
    public void testCronetVersionCreation() {
        final int major = 100;
        final int minor = 0;
        final int build = 1;
        final int patch = 33;
        final String version = String.format(Locale.US, "%d.%d.%d.%d", major, minor, build, patch);
        final CronetVersion parsedVersion = new CronetVersion(version);
        assertThat(parsedVersion.getMajorVersion()).isEqualTo(major);
        assertThat(parsedVersion.getMinorVersion()).isEqualTo(minor);
        assertThat(parsedVersion.getBuildVersion()).isEqualTo(build);
        assertThat(parsedVersion.getPatchVersion()).isEqualTo(patch);
    }

    @Test
    @SmallTest
    public void testHttpCacheModeEnum() {
        final int[] publicBuilderHttpCacheModes = {
            CronetEngine.Builder.HTTP_CACHE_DISABLED,
            CronetEngine.Builder.HTTP_CACHE_IN_MEMORY,
            CronetEngine.Builder.HTTP_CACHE_DISK_NO_HTTP,
            CronetEngine.Builder.HTTP_CACHE_DISK
        };
        for (int publicBuilderHttpCacheMode : publicBuilderHttpCacheModes) {
            HttpCacheMode cacheModeEnum =
                    HttpCacheMode.fromPublicBuilderCacheMode(publicBuilderHttpCacheMode);
            assertThat(cacheModeEnum.toPublicBuilderCacheMode())
                    .isEqualTo(publicBuilderHttpCacheMode);
        }
    }

    @Test
    @SmallTest
    public void testSetLoggerForTesting() {
        CronetLogger logger = CronetLoggerFactory.createLogger(mContext, null);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(0);
        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(0);

        // We don't care about what's being logged.
        logger.logCronetTrafficInfo(0, null);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(0);
        logger.logCronetEngineCreation(0, null, null, null);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
    }

    @Test
    @SmallTest
    public void testTelemetryDefaultEnabled() throws JSONException {
        final String url = NativeTestServer.getEchoBodyURL();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        CronetEngine engine = mTestRule.getTestFramework().startEngine();
        UrlRequest.Builder requestBuilder =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest request = requestBuilder.build();
        request.start();
        callback.blockForDone();
        assertThat(callback.mOnCanceledCalled).isFalse();
        assertThat(callback.mOnErrorCalled).isFalse();
        mTestLogger.waitForLogCronetTrafficInfo();

        // Test-logger should be bypassed.
        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
    }

    @Test
    @SmallTest
    public void testCronetEngineBuilderInitializedNotLoggedFromImpl() {
        // The test framework bypasses the logic in CronetEngine.Builder, and the logic in
        // CronetEngineBuilderImpl should not log anything since it should see the API code is up to
        // date.
        assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(0);
        mTestRule
                .getTestFramework()
                .createNewSecondaryBuilder(mTestRule.getTestFramework().getContext());
        assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(0);
    }

    @Test
    @SmallTest
    public void testCronetEngineBuilderInitializedLoggedFromImplIfApiIsTooOld() {
        var originalApiLevel = CronetEngineBuilderImpl.sApiLevel;
        try {
            CronetEngineBuilderImpl.sApiLevel = 29;
            assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(0);
            var builder =
                    mTestRule
                            .getTestFramework()
                            .createNewSecondaryBuilder(mTestRule.getTestFramework().getContext());
            // The test framework bypasses the logic in CronetEngine.Builder, so we know this is
            // coming from the impl.
            assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(1);
            var info = mTestLogger.getLastCronetEngineBuilderInitializedInfo();
            assertThat(info).isNotNull();
            assertThat(info.cronetInitializationRef).isNotEqualTo(0);
            assertThat(info.author)
                    .isEqualTo(CronetLogger.CronetEngineBuilderInitializedInfo.Author.IMPL);
            assertThat(info.engineBuilderCreatedLatencyMillis).isAtLeast(0);
            assertThat(info.source).isNotEqualTo(CronetSource.CRONET_SOURCE_UNSPECIFIED);
            assertThat(info.creationSuccessful).isTrue();
            assertThat(info.apiVersion.getMajorVersion()).isGreaterThan(0);
            assertThat(info.implVersion.getMajorVersion()).isGreaterThan(0);
            assertThat(info.uid).isGreaterThan(0);

            builder.build();
            final CronetEngineBuilderInfo builderInfo =
                    mTestLogger.getLastCronetEngineBuilderInfo();
            assertThat(builderInfo).isNotNull();
            assertThat(builderInfo.getCronetInitializationRef())
                    .isEqualTo(info.cronetInitializationRef);

            mTestLogger.waitForCronetInitializedInfo();
            var cronetInitializedInfo = mTestLogger.getLastCronetInitializedInfo();
            assertThat(cronetInitializedInfo).isNotNull();
            assertThat(cronetInitializedInfo.cronetInitializationRef)
                    .isEqualTo(info.cronetInitializationRef);
        } finally {
            CronetEngineBuilderImpl.sApiLevel = originalApiLevel;
        }
    }

    @Test
    @SmallTest
    public void testCronetEngineBuilderInitializedLoggedFromApi() {
        assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(0);
        // The test framework bypasses the logic in CronetEngine.Builder, so we have to call it
        // directly. We want to use the test framework context though for things like
        // intercepting manifest reads.
        // TODO(crbug.com/41494362): this is ugly. Ideally the test framework should be
        // refactored to stop violating the Single Responsibility Principle (e.g. Context
        // management and implementation selection should be separated)
        var builder = new CronetEngine.Builder(mTestRule.getTestFramework().getContext());
        assertThat(mTestLogger.callsToLogCronetEngineBuilderInitializedInfo()).isEqualTo(1);
        var info = mTestLogger.getLastCronetEngineBuilderInitializedInfo();
        assertThat(info).isNotNull();
        assertThat(info.cronetInitializationRef).isNotEqualTo(0);
        assertThat(info.author)
                .isEqualTo(CronetLogger.CronetEngineBuilderInitializedInfo.Author.API);
        assertThat(info.engineBuilderCreatedLatencyMillis).isAtLeast(0);
        assertThat(info.source).isNotEqualTo(CronetSource.CRONET_SOURCE_UNSPECIFIED);
        assertThat(info.creationSuccessful).isTrue();
        assertThat(info.apiVersion.getMajorVersion()).isGreaterThan(0);
        assertThat(info.implVersion.getMajorVersion()).isGreaterThan(0);
        assertThat(info.uid).isGreaterThan(0);

        builder.build();
        final CronetEngineBuilderInfo builderInfo = mTestLogger.getLastCronetEngineBuilderInfo();
        assertThat(builderInfo).isNotNull();
        assertThat(builderInfo.getCronetInitializationRef())
                .isEqualTo(info.cronetInitializationRef);

        mTestLogger.waitForCronetInitializedInfo();
        var cronetInitializedInfo = mTestLogger.getLastCronetInitializedInfo();
        assertThat(cronetInitializedInfo).isNotNull();
        assertThat(cronetInitializedInfo.cronetInitializationRef)
                .isEqualTo(info.cronetInitializationRef);
    }

    @Test
    @SmallTest
    public void testEngineCreation() throws JSONException {
        JSONObject staleDns =
                new JSONObject()
                        .put("enable", true)
                        .put("delay_ms", 0)
                        .put("allow_other_network", true)
                        .put("persist_to_disk", true)
                        .put("persist_delay_ms", 0);
        final JSONObject jsonExperimentalOptions = new JSONObject().put("StaleDNS", staleDns);
        final String experimentalOptions = jsonExperimentalOptions.toString();
        final boolean isPublicKeyPinningBypassForLocalTrustAnchorsEnabled = false;
        final String userAgent = "myUserAgent";
        final String storagePath = CronetTestRule.getTestStorage(mContext);
        final boolean isQuicEnabled = true;
        final boolean isHttp2Enabled = false;
        final boolean isBrotliEnabled = true;
        final int cacheMode = HTTP_CACHE_DISK_NO_HTTP;
        final boolean isNetworkQualityEstimatorEnabled = true;
        final int threadPriority = THREAD_PRIORITY_DEFAULT;

        mTestRule
                .getTestFramework()
                .applyEngineBuilderPatch(
                        (builder) -> {
                            builder.setExperimentalOptions(experimentalOptions);
                            builder.enablePublicKeyPinningBypassForLocalTrustAnchors(
                                    isPublicKeyPinningBypassForLocalTrustAnchorsEnabled);
                            builder.setUserAgent(userAgent);
                            builder.setStoragePath(storagePath);
                            builder.enableQuic(isQuicEnabled);
                            builder.enableHttp2(isHttp2Enabled);
                            builder.enableBrotli(isBrotliEnabled);
                            builder.enableHttpCache(cacheMode, 0);
                            builder.enableNetworkQualityEstimator(isNetworkQualityEstimatorEnabled);
                            builder.setThreadPriority(threadPriority);
                        });

        CronetEngine engine = mTestRule.getTestFramework().startEngine();
        final CronetEngineBuilderInfo builderInfo = mTestLogger.getLastCronetEngineBuilderInfo();
        final CronetVersion version = mTestLogger.getLastCronetVersion();
        final CronetSource source = mTestLogger.getLastCronetSource();

        assertThat(builderInfo.isPublicKeyPinningBypassForLocalTrustAnchorsEnabled())
                .isEqualTo(isPublicKeyPinningBypassForLocalTrustAnchorsEnabled);
        assertThat(builderInfo.getUserAgent()).isEqualTo(userAgent);
        assertThat(builderInfo.getStoragePath()).isEqualTo(storagePath);
        assertThat(builderInfo.isQuicEnabled()).isEqualTo(isQuicEnabled);
        assertThat(builderInfo.isHttp2Enabled()).isEqualTo(isHttp2Enabled);
        assertThat(builderInfo.isBrotliEnabled()).isEqualTo(isBrotliEnabled);
        assertThat(builderInfo.getHttpCacheMode()).isEqualTo(cacheMode);
        assertThat(builderInfo.getExperimentalOptions()).isEqualTo(experimentalOptions);
        assertThat(builderInfo.isNetworkQualityEstimatorEnabled())
                .isEqualTo(isNetworkQualityEstimatorEnabled);
        assertThat(builderInfo.getThreadPriority()).isEqualTo(threadPriority);
        assertThat(version.toString()).isEqualTo(ImplVersion.getCronetVersion());
        if (mTestRule.testingJavaImpl()) {
            assertThat(source).isEqualTo(CronetSource.CRONET_SOURCE_FALLBACK);
        } else {
            assertThat(source).isEqualTo(CronetSource.CRONET_SOURCE_STATICALLY_LINKED);
        }

        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(0);
    }

    @Test
    @SmallTest
    public void testCronetInitializedInfo() {
        // Creating another builder to ensure the cronet initialization ref allocation goes through
        // TestLogger.
        mTestRule
                .getTestFramework()
                .createNewSecondaryBuilder(mTestRule.getTestFramework().getContext())
                .build();
        mTestLogger.waitForCronetInitializedInfo();
        var cronetInitializedInfo = mTestLogger.getLastCronetInitializedInfo();
        assertThat(cronetInitializedInfo).isNotNull();
        assertThat(cronetInitializedInfo.cronetInitializationRef).isNotEqualTo(0);
        assertThat(cronetInitializedInfo.engineCreationLatencyMillis).isAtLeast(0);
        assertThat(cronetInitializedInfo.engineAsyncLatencyMillis).isAtLeast(0);
    }

    private void setReadHttpFlagsInManifest(boolean value) {
        Bundle metaData = new Bundle();
        metaData.putBoolean(CronetManifest.READ_HTTP_FLAGS_META_DATA_KEY, value);
        mTestRule.getTestFramework().interceptContext(new CronetManifestInterceptor(metaData));
    }

    @Test
    @SmallTest
    public void testCronetInitializedInfoHttpFlagsDisabled() {
        setReadHttpFlagsInManifest(false);

        mTestRule.getTestFramework().startEngine();
        mTestLogger.waitForCronetInitializedInfo();
        var cronetInitializedInfo = mTestLogger.getLastCronetInitializedInfo();
        assertThat(cronetInitializedInfo).isNotNull();
        assertThat(cronetInitializedInfo.httpFlagsLatencyMillis).isAtLeast(0);
        assertThat(cronetInitializedInfo.httpFlagsSuccessful).isNull();
        assertThat(cronetInitializedInfo.httpFlagsNames).isEmpty();
        assertThat(cronetInitializedInfo.httpFlagsValues).isEmpty();
    }

    @Test
    @SmallTest
    public void testCronetInitializedInfoHttpFlagsEnabled() {
        setReadHttpFlagsInManifest(true);
        mTestRule
                .getTestFramework()
                .setHttpFlags(
                        Flags.newBuilder()
                                .putFlags(
                                        "true_bool_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setBoolValue(true))
                                                .build())
                                .putFlags(
                                        "false_bool_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setBoolValue(false))
                                                .build())
                                .putFlags(
                                        "int_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setIntValue(42))
                                                .build())
                                .putFlags(
                                        "float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(42.5f))
                                                .build())
                                .putFlags(
                                        "small_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(0.000_000_001f))
                                                .build())
                                .putFlags(
                                        "large_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(1_000_000_000f))
                                                .build())
                                .putFlags(
                                        "max_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(Float.MAX_VALUE))
                                                .build())
                                .putFlags(
                                        "min_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(Float.MIN_VALUE))
                                                .build())
                                .putFlags(
                                        "negative_infinity_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(
                                                                        (Float.NEGATIVE_INFINITY)))
                                                .build())
                                .putFlags(
                                        "positive_infinity_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(
                                                                        Float.POSITIVE_INFINITY))
                                                .build())
                                .putFlags(
                                        "nan_float_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setFloatValue(Float.NaN))
                                                .build())
                                .putFlags(
                                        "string_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setStringValue(
                                                                        "string_flag_value"))
                                                .build())
                                .putFlags(
                                        "bytes_flag_name",
                                        FlagValue.newBuilder()
                                                .addConstrainedValues(
                                                        FlagValue.ConstrainedValue.newBuilder()
                                                                .setBytesValue(
                                                                        ByteString.copyFrom(
                                                                                new byte[] {
                                                                                    'b', 'y', 't',
                                                                                    'e', 's'
                                                                                })))
                                                .build())
                                .build());

        mTestRule.getTestFramework().startEngine();
        mTestLogger.waitForCronetInitializedInfo();
        var cronetInitializedInfo = mTestLogger.getLastCronetInitializedInfo();
        assertThat(cronetInitializedInfo).isNotNull();
        assertThat(cronetInitializedInfo.httpFlagsLatencyMillis).isAtLeast(0);
        assertThat(cronetInitializedInfo.httpFlagsSuccessful).isTrue();
        assertThat(cronetInitializedInfo.httpFlagsNames)
                .containsExactly(
                        // MD5("negative_infinity_float_flag_name") =
                        // 9d696a83954fce335d1e8f6a9810492b
                        0x9d696a83954fce33L, // -7104029823821951437
                        // MD5("small_float_flag_name") = ab72c47c397f6a43c613537a966219af
                        0xab72c47c397f6a43L, // -6092591308059219389
                        // MD5("bytes_flag_name") = ba9a5e9e6b90bfbb81680aa990b634eb
                        0xba9a5e9e6b90bfbbL, // -5000580401739022405
                        // MD5("true_bool_flag_name") = c9c07033b6976b0da6db6289170f57cf
                        0xc9c07033b6976b0dL, // -3909001109148570867
                        // MD5("min_float_flag_name") = d256444fcdc3c570c80a581c9693b7d1
                        0xd256444fcdc3c570L, // -3290367368202304144
                        // MD5("nan_float_flag_name") = d6ab336abb86e1898709bbc0cd39ddf5
                        0xd6ab336abb86e189L, // -2978230195069722231
                        // MD5("float_flag_name") = da91d8c7fd76cbad314db23f64d851cc
                        0xda91d8c7fd76cbadL, // -2697136348355703891
                        // MD5("false_bool_flag_name") = 054604bfb6c5dda576298f09ae47e27e
                        0x054604bfb6c5dda5L, // 379996440011070885
                        // MD5("string_flag_name") = 44c075a1c49b1a409841fd7be863b836
                        0x44c075a1c49b1a40L, // 4954088927756229184
                        // MD5("positive_infinity_float_flag_name") =
                        // 4bfaed6273d43c6f9e0b3bf6cab1ba77
                        0x4bfaed6273d43c6fL, // 5474949304128126063
                        // MD5("int_flag_name") = 57c409545ac02037d0251cffc79ac636
                        0x57c409545ac02037L, // 6324190034639462455
                        // MD5("max_float_flag_name") = 5e4417deb0623183ba34114388f0633f
                        0x5e4417deb0623183L, // 6792580383190954371
                        // MD5("large_float_flag_name") = 6bc0a810fc7df652ff363da9130fc725
                        0x6bc0a810fc7df652L // 7764390548495791698
                        )
                .inOrder();
        assertThat(cronetInitializedInfo.httpFlagsValues)
                .containsExactly(
                        // negative_infinity_float_flag_name
                        Long.MIN_VALUE,
                        // small_float_flag_name
                        1L,
                        // bytes_flag_name
                        // MD5("bytes") = 4b3a6218bb3e3a7303e8a171a60fcf92
                        0x4b3a6218bb3e3a73L,
                        // true_bool_flag_name
                        1L,
                        // min_float_flag_name
                        0L,
                        // nan_float_flag_name
                        0L,
                        // float_flag_name
                        42_500_000_000L,
                        // false_bool_flag_name
                        0L,
                        // string_flag_name
                        // MD5("string_flag_value") = de880cd0cda4184ef97ee4ad3757e5c3
                        0xde880cd0cda4184eL,
                        // positive_infinity_float_flag_name
                        Long.MAX_VALUE,
                        // int_flag_name
                        42L,
                        // max_float_flag_name
                        Long.MAX_VALUE,
                        // large_float_flag_name
                        1_000_000_000_000_000_000L)
                .inOrder();
    }

    @Test
    @SmallTest
    public void testEngineCreationAndTrafficInfoEngineId() throws Exception {
        final String url = "www.example.com";
        CronetEngine engine = mTestRule.getTestFramework().startEngine();
        final long engineId = mTestLogger.getLastCronetEngineId();

        TestUrlRequestCallback callback1 = new TestUrlRequestCallback();
        UrlRequest.Builder requestBuilder1 =
                engine.newUrlRequestBuilder(url, callback1, callback1.getExecutor());
        UrlRequest request1 = requestBuilder1.build();
        TestUrlRequestCallback callback2 = new TestUrlRequestCallback();
        UrlRequest.Builder requestBuilder2 =
                engine.newUrlRequestBuilder(url, callback2, callback2.getExecutor());
        UrlRequest request2 = requestBuilder2.build();

        request1.start();
        callback1.blockForDone();
        mTestLogger.waitForLogCronetTrafficInfo();
        final long request1Id = mTestLogger.getLastCronetRequestId();

        request2.start();
        callback2.blockForDone();
        mTestLogger.waitForLogCronetTrafficInfo();
        final long request2Id = mTestLogger.getLastCronetRequestId();

        assertThat(request1Id).isEqualTo(engineId);
        assertThat(request2Id).isEqualTo(engineId);

        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(2);
    }

    @Test
    @SmallTest
    public void testMultipleEngineCreationAndTrafficInfoEngineId() throws Exception {
        final String url = "www.example.com";
        ExperimentalCronetEngine.Builder engineBuilder =
                (ExperimentalCronetEngine.Builder)
                        mTestRule
                                .getTestFramework()
                                .createNewSecondaryBuilder(
                                        mTestRule.getTestFramework().getContext());

        CronetEngine engine1 = engineBuilder.build();
        final long engine1Id = mTestLogger.getLastCronetEngineId();
        CronetEngine engine2 = engineBuilder.build();
        final long engine2Id = mTestLogger.getLastCronetEngineId();

        try {
            TestUrlRequestCallback callback1 = new TestUrlRequestCallback();
            UrlRequest.Builder requestBuilder1 =
                    engine1.newUrlRequestBuilder(url, callback1, callback1.getExecutor());
            UrlRequest request1 = requestBuilder1.build();
            TestUrlRequestCallback callback2 = new TestUrlRequestCallback();
            UrlRequest.Builder requestBuilder2 =
                    engine2.newUrlRequestBuilder(url, callback2, callback2.getExecutor());
            UrlRequest request2 = requestBuilder2.build();

            request1.start();
            callback1.blockForDone();
            mTestLogger.waitForLogCronetTrafficInfo();
            final long request1Id = mTestLogger.getLastCronetRequestId();

            request2.start();
            callback2.blockForDone();
            mTestLogger.waitForLogCronetTrafficInfo();
            final long request2Id = mTestLogger.getLastCronetRequestId();

            assertThat(request1Id).isEqualTo(engine1Id);
            assertThat(request2Id).isEqualTo(engine2Id);

            assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(2);
            assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(2);
        } finally {
            engine1.shutdown();
            engine2.shutdown();
        }
    }

    @Test
    @SmallTest
    public void testSuccessfulRequestNative() throws Exception {
        final String url = NativeTestServer.getEchoBodyURL();
        CronetEngine engine = mTestRule.getTestFramework().startEngine();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder requestBuilder =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest request = requestBuilder.build();
        request.start();
        callback.blockForDone();
        assertThat(callback.mOnCanceledCalled).isFalse();
        assertThat(callback.mOnErrorCalled).isFalse();
        mTestLogger.waitForLogCronetTrafficInfo();

        final CronetTrafficInfo trafficInfo = mTestLogger.getLastCronetTrafficInfo();
        assertThat(trafficInfo.getRequestHeaderSizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getRequestBodySizeInBytes()).isNotEqualTo(0);
        assertThat(trafficInfo.getResponseHeaderSizeInBytes()).isNotEqualTo(0);
        assertThat(trafficInfo.getResponseBodySizeInBytes()).isNotEqualTo(0);
        assertThat(trafficInfo.getResponseStatusCode()).isEqualTo(200);
        assertThat(trafficInfo.getHeadersLatency()).isNotEqualTo(Duration.ofSeconds(0));
        assertThat(trafficInfo.getTotalLatency()).isNotEqualTo(Duration.ofSeconds(0));
        assertThat(trafficInfo.getNegotiatedProtocol()).isNotNull();
        assertThat(trafficInfo.wasConnectionMigrationAttempted()).isFalse();
        assertThat(trafficInfo.didConnectionMigrationSucceed()).isFalse();
        assertThat(trafficInfo.getTerminalState())
                .isEqualTo(CronetTrafficInfo.RequestTerminalState.SUCCEEDED);

        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
    }

    @Test
    @SmallTest
    public void testFailedRequestNative() throws Exception {
        final String url = "www.unreachable-url.com";
        CronetEngine engine = mTestRule.getTestFramework().startEngine();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        UrlRequest.Builder requestBuilder =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest request = requestBuilder.build();
        request.start();
        callback.blockForDone();
        assertThat(callback.mOnCanceledCalled).isFalse();
        assertThat(callback.mOnErrorCalled).isTrue();
        mTestLogger.waitForLogCronetTrafficInfo();

        final CronetTrafficInfo trafficInfo = mTestLogger.getLastCronetTrafficInfo();
        assertThat(trafficInfo.getRequestHeaderSizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getRequestBodySizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getResponseHeaderSizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getResponseBodySizeInBytes()).isEqualTo(0);
        // When a request fails before hitting the server all these values won't be populated in
        // the actual code. Check that the logger sets them to some known defaults before
        // logging.
        assertThat(trafficInfo.getResponseStatusCode()).isEqualTo(0);
        assertThat(trafficInfo.getNegotiatedProtocol()).isEmpty();
        assertThat(trafficInfo.wasConnectionMigrationAttempted()).isFalse();
        assertThat(trafficInfo.didConnectionMigrationSucceed()).isFalse();
        assertThat(trafficInfo.getTerminalState())
                .isEqualTo(CronetTrafficInfo.RequestTerminalState.ERROR);

        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
    }

    @Test
    @SmallTest
    public void testCanceledRequestNative() throws Exception {
        final String url = NativeTestServer.getEchoBodyURL();
        CronetEngine engine = mTestRule.getTestFramework().startEngine();

        TestUrlRequestCallback callback = new TestUrlRequestCallback();
        callback.setAutoAdvance(false);
        UrlRequest.Builder requestBuilder =
                engine.newUrlRequestBuilder(url, callback, callback.getExecutor());
        UrlRequest request = requestBuilder.build();
        request.start();
        request.cancel();
        callback.blockForDone();
        assertThat(callback.mOnCanceledCalled).isTrue();
        assertThat(callback.mOnErrorCalled).isFalse();
        mTestLogger.waitForLogCronetTrafficInfo();

        final CronetTrafficInfo trafficInfo = mTestLogger.getLastCronetTrafficInfo();
        assertThat(trafficInfo.getRequestHeaderSizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getRequestBodySizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getResponseHeaderSizeInBytes()).isEqualTo(0);
        assertThat(trafficInfo.getResponseBodySizeInBytes()).isEqualTo(0);
        // When a request fails before hitting the server all these values won't be populated in
        // the actual code. Check that the logger sets them to some known defaults before
        // logging.
        assertThat(trafficInfo.getResponseStatusCode()).isEqualTo(0);
        assertThat(trafficInfo.getNegotiatedProtocol()).isEmpty();
        assertThat(trafficInfo.wasConnectionMigrationAttempted()).isFalse();
        assertThat(trafficInfo.didConnectionMigrationSucceed()).isFalse();
        assertThat(trafficInfo.getTerminalState())
                .isEqualTo(CronetTrafficInfo.RequestTerminalState.CANCELLED);

        assertThat(mTestLogger.callsToLogCronetEngineCreation()).isEqualTo(1);
        assertThat(mTestLogger.callsToLogCronetTrafficInfo()).isEqualTo(1);
    }

    @Test
    @SmallTest
    public void testEmptyHeadersSizeNative() {
        Map<String, List<String>> headers = Collections.emptyMap();
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headers)).isEqualTo(0);
        headers = null;
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headers)).isEqualTo(0);

        CronetUrlRequest.HeadersList headersList = new CronetUrlRequest.HeadersList();
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headersList)).isEqualTo(0);
        headersList = null;
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headersList)).isEqualTo(0);
    }

    @Test
    @SmallTest
    public void testNonEmptyHeadersSizeNative() {
        Map<String, List<String>> headers =
                new HashMap<String, List<String>>() {
                    {
                        put("header1", Arrays.asList("value1", "value2")); // 7 + 6 + 6 = 19
                        put("header2", null); // 19 + 7 = 26
                        put("header3", Collections.emptyList()); // 26 + 7 + 0 = 33
                        put(null, Arrays.asList("")); // 33 + 0 + 0 = 33
                    }
                };
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headers)).isEqualTo(33);

        CronetUrlRequest.HeadersList headersList = new CronetUrlRequest.HeadersList();
        headersList.add(
                new AbstractMap.SimpleImmutableEntry<String, String>(
                        "header1", "value1") // 7 + 6 = 13
                );
        headersList.add(
                new AbstractMap.SimpleImmutableEntry<String, String>(
                        "header1", "value2") // 13 + 7 + 6 = 26
                );
        headersList.add(
                new AbstractMap.SimpleImmutableEntry<String, String>(
                        "header2", null) // 26 + 7 + 0 = 33
                );
        headersList.add(
                new AbstractMap.SimpleImmutableEntry<String, String>(null, "") // 33 + 0 + 0 = 33
                );
        assertThat(CronetUrlRequest.estimateHeadersSizeInBytes(headersList)).isEqualTo(33);
    }

    /** Records the last engine creation (and traffic info) call it has received. */
    public static final class TestLogger extends CronetLogger {
        private AtomicInteger mNextId = new AtomicInteger();
        private final AtomicInteger mCallsToLogCronetEngineBuilderInitializedInfo =
                new AtomicInteger();
        private final AtomicInteger mCallsToCronetInitializedInfo = new AtomicInteger();
        private AtomicInteger mCallsToLogCronetEngineCreation = new AtomicInteger();
        private AtomicInteger mCallsToLogCronetTrafficInfo = new AtomicInteger();
        private AtomicLong mCronetEngineId = new AtomicLong();
        private AtomicLong mCronetRequestId = new AtomicLong();
        private final AtomicReference<CronetEngineBuilderInitializedInfo>
                mCronetEngineBuilderInitializedInfo = new AtomicReference<>();
        private final AtomicReference<CronetInitializedInfo> mCronetInitializedInfo =
                new AtomicReference<>();
        private AtomicReference<CronetTrafficInfo> mTrafficInfo = new AtomicReference<>();
        private AtomicReference<CronetEngineBuilderInfo> mBuilderInfo = new AtomicReference<>();
        private AtomicReference<CronetVersion> mVersion = new AtomicReference<>();
        private AtomicReference<CronetSource> mSource = new AtomicReference<>();
        private final ConditionVariable mCronetInitializedInfoCalled = new ConditionVariable();
        private final ConditionVariable mBlock = new ConditionVariable();

        @Override
        public long generateId() {
            return mNextId.incrementAndGet();
        }

        @Override
        public void logCronetEngineBuilderInitializedInfo(CronetEngineBuilderInitializedInfo info) {
            mCallsToLogCronetEngineBuilderInitializedInfo.incrementAndGet();
            mCronetEngineBuilderInitializedInfo.set(info);
        }

        @Override
        public void logCronetInitializedInfo(CronetInitializedInfo info) {
            mCallsToCronetInitializedInfo.incrementAndGet();
            mCronetInitializedInfo.set(info);
            mCronetInitializedInfoCalled.open();
        }

        @Override
        public void logCronetEngineCreation(
                long cronetEngineId,
                CronetEngineBuilderInfo engineBuilderInfo,
                CronetVersion version,
                CronetSource source) {
            mCallsToLogCronetEngineCreation.incrementAndGet();
            mCronetEngineId.set(cronetEngineId);
            mBuilderInfo.set(engineBuilderInfo);
            mVersion.set(version);
            mSource.set(source);
        }

        @Override
        public void logCronetTrafficInfo(long cronetEngineId, CronetTrafficInfo trafficInfo) {
            mCallsToLogCronetTrafficInfo.incrementAndGet();
            mCronetRequestId.set(cronetEngineId);
            mTrafficInfo.set(trafficInfo);
            mBlock.open();
        }

        public int callsToLogCronetEngineBuilderInitializedInfo() {
            return mCallsToLogCronetEngineBuilderInitializedInfo.get();
        }

        public int callsToLogCronetTrafficInfo() {
            return mCallsToLogCronetTrafficInfo.get();
        }

        public int callsToLogCronetEngineCreation() {
            return mCallsToLogCronetEngineCreation.get();
        }

        public void waitForCronetInitializedInfo() {
            mCronetInitializedInfoCalled.block();
            mCronetInitializedInfoCalled.close();
        }

        public void waitForLogCronetTrafficInfo() {
            mBlock.block();
            mBlock.close();
        }

        public long getLastCronetEngineId() {
            return mCronetEngineId.get();
        }

        public long getLastCronetRequestId() {
            return mCronetRequestId.get();
        }

        public CronetEngineBuilderInitializedInfo getLastCronetEngineBuilderInitializedInfo() {
            return mCronetEngineBuilderInitializedInfo.get();
        }

        public CronetInitializedInfo getLastCronetInitializedInfo() {
            return mCronetInitializedInfo.get();
        }

        public CronetTrafficInfo getLastCronetTrafficInfo() {
            return mTrafficInfo.get();
        }

        public CronetEngineBuilderInfo getLastCronetEngineBuilderInfo() {
            return mBuilderInfo.get();
        }

        public CronetVersion getLastCronetVersion() {
            return mVersion.get();
        }

        public CronetSource getLastCronetSource() {
            return mSource.get();
        }
    }
}
