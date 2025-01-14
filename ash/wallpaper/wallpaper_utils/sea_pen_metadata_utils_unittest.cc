// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wallpaper/wallpaper_utils/sea_pen_metadata_utils.h"

#include <string>

#include "ash/public/cpp/test/in_process_data_decoder.h"
#include "ash/webui/common/mojom/sea_pen.mojom.h"
#include "base/files/file_path.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/test/icu_test_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time_override.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {
namespace {

std::string user_search_query = "search query";
std::string user_visible_query_text = "test template query text";
std::string user_visible_query_template = "test template title";

base::Value::Dict GetTestTemplateQueryDict(
    base::Time time = base::Time::Now()) {
  return base::Value::Dict()
      .Set("creation_time", base::TimeToValue(time))
      .Set("template_id",
           base::NumberToString(static_cast<int32_t>(
               ash::personalization_app::mojom::SeaPenTemplateId::kFlower)))
      .Set("options", base::Value::Dict()
                          .Set(base::NumberToString(static_cast<int32_t>(
                                   ash::personalization_app::mojom::
                                       SeaPenTemplateChip::kFlowerColor)),
                               base::NumberToString(static_cast<int32_t>(
                                   ash::personalization_app::mojom::
                                       SeaPenTemplateOption::kFlowerColorBlue)))
                          .Set(base::NumberToString(static_cast<int32_t>(
                                   ash::personalization_app::mojom::
                                       SeaPenTemplateChip::kFlowerType)),
                               base::NumberToString(static_cast<int32_t>(
                                   ash::personalization_app::mojom::
                                       SeaPenTemplateOption::kFlowerTypeRose))))
      .Set("user_visible_query_text", user_visible_query_text)
      .Set("user_visible_query_template", user_visible_query_template);
}

base::Value::Dict GetTestFreeformQueryDict(
    base::Time time = base::Time::Now()) {
  return base::Value::Dict()
      .Set("creation_time", base::TimeToValue(time))
      .Set("freeform_query", user_search_query);
}

base::Value::Dict GetTestInvalidTemplateQueryDict(
    const std::string& missing_field) {
  base::Value::Dict template_query_dict = GetTestTemplateQueryDict();
  if (template_query_dict.contains(missing_field)) {
    template_query_dict.Remove(missing_field);
  }
  return template_query_dict;
}

base::subtle::ScopedTimeClockOverrides CreateScopedTimeNowOverride() {
  return base::subtle::ScopedTimeClockOverrides(
      []() -> base::Time {
        base::Time fake_now;
        bool success =
            base::Time::FromString("2023-04-05T01:23:45Z", &fake_now);
        DCHECK(success);
        return fake_now;
      },
      nullptr, nullptr);
}

personalization_app::mojom::RecentSeaPenImageInfoPtr
SeaPenQueryDictToRecentImageInfo(const base::Value::Dict& dict) {
  std::string json_string = base::WriteJson(dict).value_or(std::string());
  base::test::TestFuture<personalization_app::mojom::RecentSeaPenImageInfoPtr>
      future;
  DecodeJsonMetadata(json_string, future.GetCallback());
  return future.Take();
}

class SeaPenMetadataUtilsTest : public testing::Test {
 public:
  SeaPenMetadataUtilsTest() = default;

  SeaPenMetadataUtilsTest(const SeaPenMetadataUtilsTest&) = delete;
  SeaPenMetadataUtilsTest& operator=(const SeaPenMetadataUtilsTest&) = delete;

  ~SeaPenMetadataUtilsTest() override = default;

 private:
  base::test::TaskEnvironment task_environment_;
  InProcessDataDecoder decoder_;
};

TEST_F(SeaPenMetadataUtilsTest, SeaPenTextQueryToDict) {
  auto time_override = CreateScopedTimeNowOverride();

  ash::personalization_app::mojom::SeaPenQueryPtr search_query =
      ash::personalization_app::mojom::SeaPenQuery::NewTextQuery(
          user_search_query);

  base::Value::Dict result = SeaPenQueryToDict(search_query);

  EXPECT_EQ(GetTestFreeformQueryDict(), result);
}

TEST_F(SeaPenMetadataUtilsTest, SeaPenTemplateQueryToDict) {
  auto time_override = CreateScopedTimeNowOverride();

  base::flat_map<ash::personalization_app::mojom::SeaPenTemplateChip,
                 ash::personalization_app::mojom::SeaPenTemplateOption>
      options(
          {{ash::personalization_app::mojom::SeaPenTemplateChip::kFlowerColor,
            ash::personalization_app::mojom::SeaPenTemplateOption::
                kFlowerColorBlue},
           {ash::personalization_app::mojom::SeaPenTemplateChip::kFlowerType,
            ash::personalization_app::mojom::SeaPenTemplateOption::
                kFlowerTypeRose}});
  ash::personalization_app::mojom::SeaPenQueryPtr search_query =
      ash::personalization_app::mojom::SeaPenQuery::NewTemplateQuery(
          ash::personalization_app::mojom::SeaPenTemplateQuery::New(
              ash::personalization_app::mojom::SeaPenTemplateId::kFlower,
              options,
              ash::personalization_app::mojom::SeaPenUserVisibleQuery::New(
                  user_visible_query_text, user_visible_query_template)));

  base::Value::Dict result = SeaPenQueryToDict(search_query);

  EXPECT_EQ(GetTestTemplateQueryDict(), result);
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoVerifyCreationTime) {
  const base::test::ScopedRestoreICUDefaultLocale locale("en_US");
  const base::test::ScopedRestoreDefaultTimezone la_time("America/Los_Angeles");

  base::Time test_creation_time;
  ASSERT_TRUE(base::Time::FromString("Fri, 01 Dec 2023 00:00:00 GMT",
                                     &test_creation_time));

  auto recent_image_info = SeaPenQueryDictToRecentImageInfo(
      GetTestFreeformQueryDict(test_creation_time));

  // creation time in recent_image_info should be "Nov 30, 2023" which is
  // converted from "Fri, 01 Dec 2023 00:00:00 GMT" to America/Los_Angeles time.
  EXPECT_EQ(u"Nov 30, 2023", recent_image_info->creation_time);
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoInvalidCreationTime) {
  base::Value::Dict invalid_creation_time_query_dict =
      GetTestFreeformQueryDict().Set("creation_time", "invalid creation time");
  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(invalid_creation_time_query_dict);

  EXPECT_FALSE(recent_image_info->creation_time);
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoValidTemplateData) {
  auto expected_user_visible_query =
      ash::personalization_app::mojom::SeaPenUserVisibleQuery::New(
          user_visible_query_text, user_visible_query_template);

  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(GetTestTemplateQueryDict());

  EXPECT_TRUE(recent_image_info->user_visible_query.Equals(
      expected_user_visible_query));
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoValidFreeformData) {
  auto expected_user_visible_query =
      ash::personalization_app::mojom::SeaPenUserVisibleQuery::New(
          user_search_query, std::string());

  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(GetTestFreeformQueryDict());

  EXPECT_TRUE(recent_image_info->user_visible_query.Equals(
      expected_user_visible_query));
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoMissingCreationTime) {
  base::Value::Dict invalid_template_query_dict =
      GetTestInvalidTemplateQueryDict(/*missing_field=*/"creation_time");

  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(invalid_template_query_dict);

  EXPECT_FALSE(recent_image_info);
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoMissingUserVisibleQueryText) {
  base::Value::Dict invalid_template_query_dict =
      GetTestInvalidTemplateQueryDict(
          /*missing_field=*/"user_visible_query_text");

  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(invalid_template_query_dict);

  EXPECT_FALSE(recent_image_info);
}

TEST_F(SeaPenMetadataUtilsTest,
       SeaPenQueryDictToRecentImageInfoMissingUserVisibleQueryTemplate) {
  base::Value::Dict invalid_template_query_dict =
      GetTestInvalidTemplateQueryDict(
          /*missing_field=*/"user_visible_query_template");

  auto recent_image_info =
      SeaPenQueryDictToRecentImageInfo(invalid_template_query_dict);

  EXPECT_FALSE(recent_image_info);
}

TEST_F(SeaPenMetadataUtilsTest, GetIdFromValidFilePath) {
  std::vector<std::pair<std::string, uint32_t>> cases = {
      {"97531", 97531u},
      {"24680.jpg", 24680},
      {"abcd/1234.jpg", 1234u},
      {"a/b/c/d/575757.png", 575757u}};
  for (const auto& [path, expected] : cases) {
    EXPECT_EQ(expected, GetIdFromFileName(base::FilePath(path)));
  }
}

TEST_F(SeaPenMetadataUtilsTest, GetIdFromInvalidFilePath) {
  std::vector<std::string> cases = {"a", "b.jpg", "-21", "a/21.jpg/c"};
  for (const auto& path : cases) {
    EXPECT_FALSE(GetIdFromFileName(base::FilePath(path)).has_value());
  }
}

}  // namespace
}  // namespace ash
