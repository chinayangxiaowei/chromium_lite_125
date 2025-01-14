// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/wallpaper/wallpaper_info.h"

#include <iostream>

#include "ash/public/cpp/wallpaper/online_wallpaper_params.h"
#include "ash/public/cpp/wallpaper/wallpaper_types.h"
#include "base/ranges/algorithm.h"

namespace ash {

WallpaperInfo::WallpaperInfo() {
  layout = WALLPAPER_LAYOUT_CENTER;
  type = WallpaperType::kCount;
}

WallpaperInfo::WallpaperInfo(
    const OnlineWallpaperParams& online_wallpaper_params,
    const OnlineWallpaperVariant& target_variant)
    : location(target_variant.raw_url.spec()),
      layout(online_wallpaper_params.layout),
      type(online_wallpaper_params.daily_refresh_enabled
               ? WallpaperType::kDaily
               : WallpaperType::kOnline),
      date(base::Time::Now()),
      asset_id(target_variant.asset_id),
      collection_id(online_wallpaper_params.collection_id),
      unit_id(online_wallpaper_params.unit_id),
      variants(online_wallpaper_params.variants) {}

WallpaperInfo::WallpaperInfo(
    const GooglePhotosWallpaperParams& google_photos_wallpaper_params)
    : layout(google_photos_wallpaper_params.layout), date(base::Time::Now()) {
  if (google_photos_wallpaper_params.daily_refresh_enabled) {
    type = WallpaperType::kDailyGooglePhotos;
    collection_id = google_photos_wallpaper_params.id;
  } else {
    type = WallpaperType::kOnceGooglePhotos;
    location = google_photos_wallpaper_params.id;
    dedup_key = google_photos_wallpaper_params.dedup_key;
  }
}

WallpaperInfo::WallpaperInfo(const std::string& in_location,
                             WallpaperLayout in_layout,
                             WallpaperType in_type,
                             const base::Time& in_date,
                             const std::string& in_user_file_path)
    : location(in_location),
      user_file_path(in_user_file_path),
      layout(in_layout),
      type(in_type),
      date(in_date) {}

WallpaperInfo::WallpaperInfo(const WallpaperInfo& other) = default;
WallpaperInfo& WallpaperInfo::operator=(const WallpaperInfo& other) = default;

WallpaperInfo::WallpaperInfo(WallpaperInfo&& other) = default;
WallpaperInfo& WallpaperInfo::operator=(WallpaperInfo&& other) = default;

bool WallpaperInfo::MatchesSelection(const WallpaperInfo& other) const {
  // |location| are skipped on purpose in favor of |unit_id| as
  // online wallpapers can vary across devices due to their color mode. Other
  // wallpaper types still require location to be equal.
  switch (type) {
    case WallpaperType::kOnline:
    case WallpaperType::kDaily:
      return type == other.type && layout == other.layout &&
             collection_id == other.collection_id && unit_id == other.unit_id &&
             base::ranges::equal(variants, other.variants);
    case WallpaperType::kOnceGooglePhotos:
    case WallpaperType::kDailyGooglePhotos:
      return location == other.location && layout == other.layout &&
             collection_id == other.collection_id;
    case WallpaperType::kCustomized:
      // |location| is skipped for customized wallpaper as it includes files id
      // which is different between devices even it refers to the same file.
      // Comparing |user_file_path| that contains the absolute path should be
      // enough.
      return type == other.type && layout == other.layout &&
             user_file_path == other.user_file_path;
    case WallpaperType::kSeaPen:
    case WallpaperType::kDefault:
    case WallpaperType::kPolicy:
    case WallpaperType::kThirdParty:
    case WallpaperType::kDevice:
    case WallpaperType::kOneShot:
    case WallpaperType::kOobe:
    case WallpaperType::kCount:
      return type == other.type && layout == other.layout &&
             location == other.location;
  }
}

bool WallpaperInfo::MatchesAsset(const WallpaperInfo& other) const {
  if (!MatchesSelection(other))
    return false;

  switch (type) {
    case WallpaperType::kOnline:
    case WallpaperType::kDaily:
      return location == other.location;
    case WallpaperType::kOnceGooglePhotos:
    case WallpaperType::kDailyGooglePhotos:
    case WallpaperType::kCustomized:
    case WallpaperType::kDefault:
    case WallpaperType::kPolicy:
    case WallpaperType::kThirdParty:
    case WallpaperType::kDevice:
    case WallpaperType::kOneShot:
    case WallpaperType::kOobe:
    case WallpaperType::kSeaPen:
    case WallpaperType::kCount:
      return true;
  }
}

base::Value::Dict WallpaperInfo::ToDict() const {
  base::Value::Dict wallpaper_info_dict;
  if (asset_id.has_value()) {
    wallpaper_info_dict.Set(kNewWallpaperAssetIdNodeName,
                            base::NumberToString(asset_id.value()));
  }
  if (unit_id.has_value()) {
    wallpaper_info_dict.Set(kNewWallpaperUnitIdNodeName,
                            base::NumberToString(unit_id.value()));
  }
  base::Value::List online_wallpaper_variant_list;
  for (const auto& variant : variants) {
    base::Value::Dict online_wallpaper_variant_dict;
    online_wallpaper_variant_dict.Set(kNewWallpaperAssetIdNodeName,
                                      base::NumberToString(variant.asset_id));
    online_wallpaper_variant_dict.Set(kOnlineWallpaperUrlNodeName,
                                      variant.raw_url.spec());
    online_wallpaper_variant_dict.Set(kOnlineWallpaperTypeNodeName,
                                      static_cast<int>(variant.type));
    online_wallpaper_variant_list.Append(
        std::move(online_wallpaper_variant_dict));
  }

  wallpaper_info_dict.Set(kNewWallpaperVariantListNodeName,
                          std::move(online_wallpaper_variant_list));
  wallpaper_info_dict.Set(kNewWallpaperCollectionIdNodeName, collection_id);
  // TODO(skau): Change time representation to TimeToValue.
  wallpaper_info_dict.Set(
      kNewWallpaperDateNodeName,
      base::NumberToString(date.ToDeltaSinceWindowsEpoch().InMicroseconds()));
  if (dedup_key) {
    wallpaper_info_dict.Set(kNewWallpaperDedupKeyNodeName, dedup_key.value());
  }
  wallpaper_info_dict.Set(kNewWallpaperLocationNodeName, location);
  wallpaper_info_dict.Set(kNewWallpaperUserFilePathNodeName, user_file_path);
  wallpaper_info_dict.Set(kNewWallpaperLayoutNodeName, layout);
  wallpaper_info_dict.Set(kNewWallpaperTypeNodeName, static_cast<int>(type));
  return wallpaper_info_dict;
}

WallpaperInfo::~WallpaperInfo() = default;

std::ostream& operator<<(std::ostream& os, const WallpaperInfo& info) {
  os << "WallpaperInfo:" << std::endl;
  os << "  location: " << info.location << std::endl;
  os << "  user_file_path: " << info.user_file_path << std::endl;
  os << "  layout: " << info.layout << std::endl;
  os << "  type: " << static_cast<int>(info.type) << std::endl;
  os << "  date: " << info.date << std::endl;
  os << "  dedup_key: " << info.dedup_key.value_or("") << std::endl;
  os << "  asset_id: " << info.asset_id.value_or(-1) << std::endl;
  os << "  collection_id: " << info.collection_id << std::endl;
  os << "  unit_id: " << info.unit_id.value_or(-1) << std::endl;
  os << "  variants_size: " << info.variants.size() << std::endl;
  return os;
}

}  // namespace ash
