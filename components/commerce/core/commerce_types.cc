// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/commerce/core/commerce_types.h"

namespace commerce {

ProductInfo::ProductInfo() = default;
ProductInfo::ProductInfo(const ProductInfo&) = default;
ProductInfo& ProductInfo::operator=(const ProductInfo&) = default;
ProductInfo::~ProductInfo() = default;

ProductSpecifications::ProductSpecifications() = default;

ProductSpecifications::ProductSpecifications(const ProductSpecifications&) =
    default;
ProductSpecifications::~ProductSpecifications() = default;

ProductSpecifications::Product::Product() = default;
ProductSpecifications::Product::Product(const ProductSpecifications::Product&) =
    default;
ProductSpecifications::Product::~Product() = default;

MerchantInfo::MerchantInfo() = default;
MerchantInfo::MerchantInfo(const MerchantInfo&) = default;
MerchantInfo& MerchantInfo::operator=(const MerchantInfo&) = default;
MerchantInfo::MerchantInfo(MerchantInfo&&) = default;
MerchantInfo::~MerchantInfo() = default;

PriceInsightsInfo::PriceInsightsInfo() = default;
PriceInsightsInfo::PriceInsightsInfo(const PriceInsightsInfo&) = default;
PriceInsightsInfo& PriceInsightsInfo::operator=(const PriceInsightsInfo&) =
    default;
PriceInsightsInfo::~PriceInsightsInfo() = default;

DiscountInfo::DiscountInfo() = default;
DiscountInfo::DiscountInfo(const DiscountInfo&) = default;
DiscountInfo& DiscountInfo::operator=(const DiscountInfo&) = default;
DiscountInfo::~DiscountInfo() = default;

UrlInfo::UrlInfo() = default;
UrlInfo::UrlInfo(const UrlInfo&) = default;
UrlInfo& UrlInfo::operator=(const UrlInfo& other) = default;
UrlInfo::~UrlInfo() = default;

ParcelTrackingStatus::ParcelTrackingStatus() = default;
ParcelTrackingStatus::ParcelTrackingStatus(const ParcelTrackingStatus&) =
    default;
ParcelTrackingStatus& ParcelTrackingStatus::operator=(
    const ParcelTrackingStatus&) = default;
ParcelTrackingStatus::~ParcelTrackingStatus() = default;
ParcelTrackingStatus::ParcelTrackingStatus(const ParcelStatus& parcel_status) {
  carrier = parcel_status.parcel_identifier().carrier();
  tracking_id = parcel_status.parcel_identifier().tracking_id();
  state = parcel_status.parcel_state();
  tracking_url = GURL(parcel_status.tracking_url());
  estimated_delivery_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(parcel_status.estimated_delivery_time_usec()));
}

}  // namespace commerce
