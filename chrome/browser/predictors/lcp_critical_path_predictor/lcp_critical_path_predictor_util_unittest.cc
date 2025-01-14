// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/lcp_critical_path_predictor/lcp_critical_path_predictor_util.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/predictors/resource_prefetch_predictor_tables.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"

namespace predictors {

TEST(IsValidLcppStatTest, Empty) {
  LcppStat lcpp_stat;
  EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
}

TEST(IsValidLcppStatTest, LcpElementLocatorStat) {
  {
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    auto* bucket = locator_stat->add_lcp_element_locator_buckets();
    bucket->set_lcp_element_locator("fake");
    bucket->set_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Without the repeated field.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative other frequency is invalid.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Nothing in an entry is invalid.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    locator_stat->add_lcp_element_locator_buckets();  // allocate a bucket.
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    auto* bucket = locator_stat->add_lcp_element_locator_buckets();
    bucket->set_lcp_element_locator("fake");
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No element locator in an entry is invalid.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    auto* bucket = locator_stat->add_lcp_element_locator_buckets();
    bucket->set_frequency(0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
    locator_stat->set_other_bucket_frequency(0.1);
    auto* bucket = locator_stat->add_lcp_element_locator_buckets();
    bucket->set_lcp_element_locator("fake");
    bucket->set_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
}

TEST(IsValidLcppStatTest, LcpScriptUrlStat) {
  {
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/script.js", 0.1});
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Without the map field.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative other frequency is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/script.js", -0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Empty URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Invalid URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"invalid url", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No HTTP/HTTPS URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"wss://example.com/", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Too long URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/" +
             std::string(ResourcePrefetchPredictorTables::kMaxStringLength,
                         'a'),
         0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
}

TEST(IsValidLcppStatTest, FetchedFontUrlStat) {
  {
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com/a.woff", 0.1});
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Without the map field.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative other frequency is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com/a.woff", -0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Empty URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Invalid URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"invalid url", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No HTTP/HTTPS URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"wss://example.com/", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Too long URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/" +
             std::string(ResourcePrefetchPredictorTables::kMaxStringLength,
                         'a'),
         0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
}

TEST(IsValidLcppStatTest, PreconnectOriginsStat) {
  {
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com", 0.1});
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Without the map field.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative other frequency is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com", -0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Empty URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Invalid URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"invalid url", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No HTTP/HTTPS URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"wss://example.com/", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Too long URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/" +
             std::string(ResourcePrefetchPredictorTables::kMaxStringLength,
                         'a'),
         0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
}

TEST(IsValidLcppStatTest, DeferUnusedPreloads) {
  {
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/unused.png", 0.1});
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Without the map field.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative other frequency is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(-0.1);
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Negative frequency in an entry is invalid.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/unused.png", -0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Empty URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Invalid URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"invalid url", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // No HTTP/HTTPS URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"wss://example.com/", 0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
  {  // Too long URL in an entry.
    LcppStat lcpp_stat;
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/" +
             std::string(ResourcePrefetchPredictorTables::kMaxStringLength,
                         'a'),
         0.1});
    EXPECT_FALSE(IsValidLcppStat(lcpp_stat));
  }
}

TEST(IsValidLcppStatTest, MixedPattern) {
  LcppStat lcpp_stat;
  auto* locator_stat = lcpp_stat.mutable_lcp_element_locator_stat();
  locator_stat->set_other_bucket_frequency(0.1);
  {
    auto* bucket = locator_stat->add_lcp_element_locator_buckets();
    bucket->set_lcp_element_locator("fake");
    bucket->set_frequency(0.1);
  }
  {
    auto* stat = lcpp_stat.mutable_lcp_script_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/script.js", 0.1});
  }
  {
    auto* stat = lcpp_stat.mutable_fetched_font_url_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com/a.woff", 0.1});
  }
  {
    auto* stat = lcpp_stat.mutable_preconnect_origin_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert({"https://example.com", 0.1});
  }
  {
    auto* stat = lcpp_stat.mutable_unused_preload_stat();
    stat->set_other_bucket_frequency(0.1);
    stat->mutable_main_buckets()->insert(
        {"https://example.com/unused.png", 0.1});
  }
  EXPECT_TRUE(IsValidLcppStat(lcpp_stat));
}

TEST(PredictFetchedFontUrls, Empty) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPFontURLPredictor, {}}}, {});
  LcppData lcpp_data;
  EXPECT_EQ(std::vector<GURL>(), PredictFetchedFontUrls(lcpp_data));
}

TEST(PredictFetchedFontUrls, Simple) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPFontURLPredictor,
        {{blink::features::kLCPPFontURLPredictorFrequencyThreshold.name, "0.5"},
         {blink::features::kLCPPFontURLPredictorMaxPreloadCount.name, "10"}}}},
      {});
  LcppData lcpp_data;
  lcpp_data.mutable_lcpp_stat()
      ->mutable_fetched_font_url_stat()
      ->mutable_main_buckets()
      ->insert({"https://example.com/a.woff", 0.9});
  std::vector<GURL> expected;
  expected.emplace_back("https://example.com/a.woff");
  EXPECT_EQ(expected, PredictFetchedFontUrls(lcpp_data));
}

TEST(PredictFetchedFontUrls, BrokenFontNames) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPFontURLPredictor,
        {{blink::features::kLCPPFontURLPredictorFrequencyThreshold.name, "0.5"},
         {blink::features::kLCPPFontURLPredictorMaxPreloadCount.name, "10"}}}},
      {});
  LcppData lcpp_data;
  auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                           ->mutable_fetched_font_url_stat()
                           ->mutable_main_buckets();
  // Duplicated.
  main_buckets->insert({"https://example.com/a.woff", 0.9});
  main_buckets->insert({"https://example.com/a.woff", 0.8});
  main_buckets->insert({"https://example.com/a.woff", 0.7});
  main_buckets->insert({"https://example.com/a.woff", 0.6});
  // Not an HTTP/HTTPS.
  main_buckets->insert({"wss://example.com/", 0.9});
  std::vector<GURL> expected;
  expected.emplace_back("https://example.com/a.woff");
  EXPECT_EQ(expected, PredictFetchedFontUrls(lcpp_data));
}

TEST(PredictFetchedFontUrls, Threshold) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPFontURLPredictor,
        {{blink::features::kLCPPFontURLPredictorFrequencyThreshold.name, "0.5"},
         {blink::features::kLCPPFontURLPredictorMaxPreloadCount.name, "10"}}}},
      {});
  LcppData lcpp_data;
  auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                           ->mutable_fetched_font_url_stat()
                           ->mutable_main_buckets();
  main_buckets->insert({"https://example.com/a.woff", 0.9});
  main_buckets->insert({"https://example.com/b.woff", 0.1});
  std::vector<GURL> expected;
  expected.emplace_back("https://example.com/a.woff");
  EXPECT_EQ(expected, PredictFetchedFontUrls(lcpp_data));
}

TEST(PredictFetchedFontUrls, MaxUrls) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{blink::features::kLCPPFontURLPredictor,
          {{blink::features::kLCPPFontURLPredictorFrequencyThreshold.name,
            "0.5"},
           {blink::features::kLCPPFontURLPredictorMaxPreloadCount.name, "1"}}}},
        {});
    LcppData lcpp_data;
    auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                             ->mutable_fetched_font_url_stat()
                             ->mutable_main_buckets();
    main_buckets->insert({"https://example.com/a.woff", 0.9});
    main_buckets->insert({"https://example.com/b.woff", 0.8});
    std::vector<GURL> expected;
    expected.emplace_back("https://example.com/a.woff");
    EXPECT_EQ(expected, PredictFetchedFontUrls(lcpp_data));
  }
  {  // Use MaxUrls as a kill switch.
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{blink::features::kLCPPFontURLPredictor,
          {{blink::features::kLCPPFontURLPredictorFrequencyThreshold.name,
            "0.5"},
           {blink::features::kLCPPFontURLPredictorMaxPreloadCount.name, "0"}}}},
        {});
    LcppData lcpp_data;
    auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                             ->mutable_fetched_font_url_stat()
                             ->mutable_main_buckets();
    main_buckets->insert({"https://example.com/a.woff", 0.9});
    main_buckets->insert({"https://example.com/b.woff", 0.8});
    std::vector<GURL> expected;
    EXPECT_EQ(expected, PredictFetchedFontUrls(lcpp_data));
  }
}

TEST(PredictFetchedSubresourceUrls, Empty) {
  EXPECT_EQ(std::vector<GURL>(), PredictFetchedSubresourceUrls({}));
}

TEST(PredictFetchedSubresourceUrls, SingleEntry) {
  LcppData lcpp_data;
  lcpp_data.mutable_lcpp_stat()
      ->mutable_fetched_subresource_url_stat()
      ->mutable_main_buckets()
      ->insert({"https://example.com/a.jpeg", 0.9});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/a.jpeg")}),
            PredictFetchedSubresourceUrls(lcpp_data));
}

TEST(PredictFetchedSubresourceUrls, SortedByFrequencyInDescendingOrder) {
  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_fetched_subresource_url_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com/c.jpeg", 0.1});
  buckets->insert({"https://example.com/a.jpeg", 0.3});
  buckets->insert({"https://example.com/b.jpeg", 0.2});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/a.jpeg"),
                               GURL("https://example.com/b.jpeg"),
                               GURL("https://example.com/c.jpeg")}),
            PredictFetchedSubresourceUrls(lcpp_data));
}

TEST(PredictFetchedSubresourceUrls, FilterUrls) {
  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_fetched_subresource_url_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com/a.jpeg", 0.1});
  buckets->insert({"https://example.com/b.jpeg", 0.2});
  // Not an HTTP/HTTPS.
  buckets->insert({"file://example.com/c.jpeg", 0.7});
  // Not an URL.
  buckets->insert({"d.jpeg", 0.8});
  EXPECT_EQ(4U, buckets->size());
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/b.jpeg"),
                               GURL("https://example.com/a.jpeg")}),
            PredictFetchedSubresourceUrls(lcpp_data));
}

TEST(PredictPreconnectableOrigins, Empty) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPAutoPreconnectLcpOrigin, {}}}, {});
  LcppData lcpp_data;
  EXPECT_EQ(std::vector<GURL>(), PredictPreconnectableOrigins(lcpp_data));
}

TEST(PredictPreconnectableOrigins, Simple) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPAutoPreconnectLcpOrigin,
        {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.5"},
         {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
          "10"}}}},
      {});
  LcppData lcpp_data;
  lcpp_data.mutable_lcpp_stat()
      ->mutable_preconnect_origin_stat()
      ->mutable_main_buckets()
      ->insert({"https://example.com", 0.9});
  std::vector<GURL> expected;
  expected.emplace_back("https://example.com");
  EXPECT_EQ(expected, PredictPreconnectableOrigins(lcpp_data));
}

TEST(PredictPreconnectableOrigins, SortedByFrequencyInDescendingOrder) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPAutoPreconnectLcpOrigin,
        {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.1"},
         {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
          "10"}}}},
      {});
  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_preconnect_origin_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com", 0.1});
  buckets->insert({"https://example2.com", 0.3});
  buckets->insert({"https://example3.com", 0.2});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example2.com"),
                               GURL("https://example3.com"),
                               GURL("https://example.com")}),
            PredictPreconnectableOrigins(lcpp_data));
}

TEST(PredictPreconnectableOrigins, Threshold) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPAutoPreconnectLcpOrigin,
        {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.5"},
         {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
          "10"}}}},
      {});
  LcppData lcpp_data;
  auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                           ->mutable_preconnect_origin_stat()
                           ->mutable_main_buckets();
  main_buckets->insert({"https://example1.com", 0.9});
  main_buckets->insert({"https://example2.com", 0.1});
  std::vector<GURL> expected;
  expected.emplace_back("https://example1.com");
  EXPECT_EQ(expected, PredictPreconnectableOrigins(lcpp_data));
}

TEST(PredictPreconnectableOrigins, MaxUrls) {
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{blink::features::kLCPPAutoPreconnectLcpOrigin,
          {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.5"},
           {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
            "1"}}}},
        {});
    LcppData lcpp_data;
    auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                             ->mutable_preconnect_origin_stat()
                             ->mutable_main_buckets();
    main_buckets->insert({"https://example.com", 0.9});
    main_buckets->insert({"https://example1.com", 0.8});
    std::vector<GURL> expected;
    expected.emplace_back("https://example.com");
    EXPECT_EQ(expected, PredictPreconnectableOrigins(lcpp_data));
  }
  {  // Use MaxUrls as a kill switch.
    base::test::ScopedFeatureList feature_list;
    feature_list.InitWithFeaturesAndParameters(
        {{blink::features::kLCPPAutoPreconnectLcpOrigin,
          {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.5"},
           {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
            "0"}}}},
        {});
    LcppData lcpp_data;
    auto* main_buckets = lcpp_data.mutable_lcpp_stat()
                             ->mutable_preconnect_origin_stat()
                             ->mutable_main_buckets();
    main_buckets->insert({"https://example1.com", 0.9});
    main_buckets->insert({"https://example2.com", 0.8});
    std::vector<GURL> expected;
    EXPECT_EQ(expected, PredictPreconnectableOrigins(lcpp_data));
  }
}

TEST(PredictPreconnectableOrigins, FilterUrls) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPAutoPreconnectLcpOrigin,
        {{blink::features::kLCPPAutoPreconnectFrequencyThreshold.name, "0.5"},
         {blink::features::kkLCPPAutoPreconnectMaxPreconnectOriginsCount.name,
          "10"}}}},
      {});
  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_preconnect_origin_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example1.com", 0.9});
  buckets->insert({"https://example2.com", 0.8});
  // Not an HTTP/HTTPS.
  buckets->insert({"file://example.com", 0.7});
  // Not an URL.
  buckets->insert({"d.jpeg", 0.8});
  EXPECT_EQ(4U, buckets->size());
  EXPECT_EQ(std::vector<GURL>(
                {GURL("https://example1.com"), GURL("https://example2.com")}),
            PredictPreconnectableOrigins(lcpp_data));
}

TEST(PredictUnusedPreloads, Empty) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPDeferUnusedPreload,
        {{blink::features::kLCPPDeferUnusedPreloadFrequencyThreshold.name,
          "0.5"}}}},
      {});

  EXPECT_EQ(std::vector<GURL>(), PredictUnusedPreloads({}));
}

TEST(PredictUnusedPreloads, SingleEntry) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPDeferUnusedPreload,
        {{blink::features::kLCPPDeferUnusedPreloadFrequencyThreshold.name,
          "0.5"}}}},
      {});

  LcppData lcpp_data;
  lcpp_data.mutable_lcpp_stat()
      ->mutable_unused_preload_stat()
      ->mutable_main_buckets()
      ->insert({"https://example.com/a.jpeg", 0.9});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/a.jpeg")}),
            PredictUnusedPreloads(lcpp_data));
}

TEST(PredictUnusedPreloads, SortedByFrequencyInDescendingOrder) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPDeferUnusedPreload,
        {{blink::features::kLCPPDeferUnusedPreloadFrequencyThreshold.name,
          "0"}}}},
      {});

  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_unused_preload_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com/c.jpeg", 0.1});
  buckets->insert({"https://example.com/a.jpeg", 0.3});
  buckets->insert({"https://example.com/b.jpeg", 0.2});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/a.jpeg"),
                               GURL("https://example.com/b.jpeg"),
                               GURL("https://example.com/c.jpeg")}),
            PredictUnusedPreloads(lcpp_data));
}

TEST(PredictUnusedPreloads, FilterUrls) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPDeferUnusedPreload,
        {{blink::features::kLCPPDeferUnusedPreloadFrequencyThreshold.name,
          "0"}}}},
      {});

  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_unused_preload_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com/a.jpeg", 0.1});
  buckets->insert({"https://example.com/b.jpeg", 0.2});
  // Not an HTTP/HTTPS.
  buckets->insert({"file://example.com/c.jpeg", 0.7});
  // Not an URL.
  buckets->insert({"d.jpeg", 0.8});
  EXPECT_EQ(4U, buckets->size());
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/b.jpeg"),
                               GURL("https://example.com/a.jpeg")}),
            PredictUnusedPreloads(lcpp_data));
}

TEST(PredictUnusedPreloads, Threshold) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{blink::features::kLCPPDeferUnusedPreload,
        {{blink::features::kLCPPDeferUnusedPreloadFrequencyThreshold.name,
          "0.5"}}}},
      {});

  LcppData lcpp_data;
  auto* buckets = lcpp_data.mutable_lcpp_stat()
                      ->mutable_unused_preload_stat()
                      ->mutable_main_buckets();
  buckets->insert({"https://example.com/a.jpeg", 0.9});
  buckets->insert({"https://example.com/b.jpeg", 0.1});
  EXPECT_EQ(std::vector<GURL>({GURL("https://example.com/a.jpeg")}),
            PredictUnusedPreloads(lcpp_data));
}

TEST(LcppKeyTest, InvalidURLs) {
  const std::string invalid_urls[] = {
      // Invalid urls
      "http://?k=v",
      "http:://google.com",
      "http://google.com:12three45",
      "://google.com",
      "path",
      "",                  // Empty
      "file://server:0",   // File
      "ftp://server",      // Ftp
      "http://localhost",  // Localhost
      "http://127.0.0.1",  // Localhost
      "https://example" +
          std::string(ResourcePrefetchPredictorTables::kMaxStringLength, 'a') +
          ".test/",  // Too long
  };

  for (auto& invalid_url : invalid_urls) {
    const GURL url(invalid_url);
    EXPECT_FALSE(IsURLValidForLcpp(url)) << invalid_url;
  }
}

void TestGetLCPPDatabaseKey(
    const std::vector<std::pair<std::string, std::string>>& url_keys,
    const base::Location& location = FROM_HERE) {
  for (const auto& url_key : url_keys) {
    const GURL url(url_key.first);
    EXPECT_TRUE(IsURLValidForLcpp(url)) << location.ToString() << url_key.first;
    const std::optional<std::string> key = GetLCPPDatabaseKey(url);
    EXPECT_TRUE(key.has_value()) << location.ToString() << url_key.first;
    EXPECT_EQ(url_key.second, *key) << location.ToString() << url_key.first;
  }
}

TEST(LcppKeyTest, GetLCPPDatabaseKey) {
  const std::vector<std::pair<std::string, std::string>> url_keys = {
      {"http://a.test", "a.test"},
      {"http://a.test/", "a.test"},
      {"http://a.test/foo", "a.test"},
      {"http://a.test/bar?q=c", "a.test"},
      {"http://user:pass@a.test:99/foo;bar?q=a#ref", "a.test"},
  };

  TestGetLCPPDatabaseKey(url_keys);
}

size_t GetLCPPMultipleKeyMaxPathLength() {
  static const size_t max_length = base::checked_cast<size_t>(
      blink::features::kLCPPMultipleKeyMaxPathLength.Get());
  return max_length;
}

TEST(LcppMultipleKeyTest, GetLCPPDatabaseKey) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeature(blink::features::kLCPPMultipleKey);

  const std::string long_host =
      std::string(ResourcePrefetchPredictorTables::kMaxStringLength - 10, 'a') +
      ".test";
  const size_t max_path_length = GetLCPPMultipleKeyMaxPathLength();
  const std::string long_path = "/" + std::string(max_path_length - 1, 'b');
  const std::string too_long_path =
      "/" + std::string(max_path_length + 1, 'c') + "/bar";
  const std::vector<std::pair<std::string, std::string>> url_keys = {
      {"http://a.test", "a.test"},
      {"http://user:pass@a.test:99/foo;bar?q=a#ref", "a.test/foo;bar"},
      {"http://a.test/", "a.test"},
      {"http://a.test/foo.html", "a.test"},
      {"http://a.test/foo", "a.test/foo"},
      {"http://a.test/foo/", "a.test/foo"},
      {"http://a.test/foo/bar", "a.test/foo"},
      {"http://a.test/foo/bar/", "a.test/foo"},
      {"http://a.test/foo/bar/baz.com", "a.test/foo"},
      {"http://a.test/bar?q=c", "a.test/bar"},
      {"http://a.test/foo/bar?q=c", "a.test/foo"},
      {"http://a.test" + long_path, "a.test" + long_path},
      {"http://a.test" + long_path + "/bar", "a.test" + long_path},
      {"http://a.test" + long_path + "bar", "a.test"},
      {"http://" + long_host + "/bar", long_host + "/bar"},
      // Both valid but if the concated key is too long, take only host.
      {"http://" + long_host + long_path, long_host},
      // Too long path is ignored.
      {"http://a.test" + too_long_path, "a.test"},
      // Invalid length path in subdirectory is also ignored.
      {"http://a.test/bar" + too_long_path, "a.test/bar"}};

  TestGetLCPPDatabaseKey(url_keys);
}

}  // namespace predictors
