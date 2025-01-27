// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/plus_addresses/webdata/plus_address_sync_util.h"

#include "base/check.h"
#include "components/plus_addresses/plus_address_types.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/plus_address_specifics.pb.h"

namespace plus_addresses {

PlusAddressSyncDataChange::PlusAddressSyncDataChange(Type type,
                                                     PlusProfile profile)
    : type_(type), profile_(std::move(profile)) {}
PlusAddressSyncDataChange::PlusAddressSyncDataChange(
    const PlusAddressSyncDataChange& other) = default;
PlusAddressSyncDataChange& PlusAddressSyncDataChange::operator=(
    const PlusAddressSyncDataChange& change) = default;
PlusAddressSyncDataChange::~PlusAddressSyncDataChange() = default;

PlusProfile PlusProfileFromEntityData(const syncer::EntityData& entity_data) {
  CHECK(entity_data.specifics.has_plus_address());
  sync_pb::PlusAddressSpecifics specifics =
      entity_data.specifics.plus_address();
  return PlusProfile{.profile_id = specifics.profile_id(),
                     .facet = specifics.facet(),
                     .plus_address = specifics.plus_email().email_address(),
                     .is_confirmed = true};
}

syncer::EntityData EntityDataFromPlusProfile(const PlusProfile& profile) {
  // Sync code should only operate on confirmed profiles.
  CHECK(profile.is_confirmed);
  syncer::EntityData entity_data;
  sync_pb::PlusAddressSpecifics* specifics =
      entity_data.specifics.mutable_plus_address();
  specifics->set_profile_id(profile.profile_id);
  specifics->set_facet(profile.facet);
  specifics->mutable_plus_email()->set_email_address(profile.plus_address);
  return entity_data;
}

}  // namespace plus_addresses
