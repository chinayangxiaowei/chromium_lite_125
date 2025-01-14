// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/plus_addresses/plus_address_jit_allocator.h"

#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/types/expected.h"
#include "components/plus_addresses/features.h"
#include "components/plus_addresses/mock_plus_address_http_client.h"
#include "components/plus_addresses/plus_address_allocator.h"
#include "components/plus_addresses/plus_address_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace plus_addresses {

namespace {

using ::testing::_;
using ::testing::Message;
using ::testing::NiceMock;

// Shorthands for common errors that the allocator can throw.
const PlusProfileOrError kNotSupportedError =
    base::unexpected(PlusAddressRequestError(
        PlusAddressRequestErrorType::kRequestNotSupportedError));
const PlusProfileOrError kMaxRefreshesReachedError = base::unexpected(
    PlusAddressRequestError(PlusAddressRequestErrorType::kMaxRefreshesReached));

url::Origin GetSampleOrigin1() {
  return url::Origin::Create(GURL("https://example1.org"));
}

url::Origin GetSampleOrigin2() {
  return url::Origin::Create(GURL("https://another-example.co.uk"));
}

}  // namespace

class PlusAddressJitAllocatorRefreshTest : public ::testing::Test {
 public:
  PlusAddressJitAllocatorRefreshTest() : allocator_(&http_client_) {}

 protected:
  PlusAddressJitAllocator& allocator() { return allocator_; }
  MockPlusAddressHttpClient& http_client() { return http_client_; }

 private:
  base::test::ScopedFeatureList feature_list_{features::kPlusAddressRefresh};

  NiceMock<MockPlusAddressHttpClient> http_client_;
  PlusAddressJitAllocator allocator_;
};

// Tests that refreshing is disabled when the feature is turned off.
TEST_F(PlusAddressJitAllocatorRefreshTest, RefreshDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kPlusAddressRefresh);

  EXPECT_FALSE(allocator().IsRefreshingSupported(GetSampleOrigin1()));
}

// Tests that the allocator translates the `AllocationMode` properly into the
// `refresh` parameter of the client.
TEST_F(PlusAddressJitAllocatorRefreshTest, RefreshParameterPassedOn) {
  EXPECT_CALL(http_client(),
              ReservePlusAddress(GetSampleOrigin1(), /*refresh=*/false, _));
  EXPECT_CALL(http_client(),
              ReservePlusAddress(GetSampleOrigin1(), /*refresh=*/true, _));
  EXPECT_CALL(http_client(),
              ReservePlusAddress(GetSampleOrigin2(), /*refresh=*/false, _));

  allocator().AllocatePlusAddress(GetSampleOrigin1(),
                                  PlusAddressAllocator::AllocationMode::kAny,
                                  base::DoNothing());
  allocator().AllocatePlusAddress(
      GetSampleOrigin1(), PlusAddressAllocator::AllocationMode::kNewPlusAddress,
      base::DoNothing());
  allocator().AllocatePlusAddress(GetSampleOrigin2(),
                                  PlusAddressAllocator::AllocationMode::kAny,
                                  base::DoNothing());
}

// Tests that refreshing is only allowed `kMaxPlusAddressRefreshesPerOrigin`
// times per origin.
TEST_F(PlusAddressJitAllocatorRefreshTest, RefreshLimit) {
  // Note: In practice, this would be a different profile with each call - but
  // the test does not need to reproduce this level of fidelity.
  const PlusProfile kSampleProfile{.plus_address = "plus+plus123@plus.com"};
  ON_CALL(http_client(), ReservePlusAddress(_, /*refresh=*/true, _))
      .WillByDefault([&kSampleProfile](const url::Origin& origin, bool refresh,
                                       PlusAddressRequestCallback cb) {
        std::move(cb).Run(kSampleProfile);
      });

  for (int i = 0; i < PlusAddressAllocator::kMaxPlusAddressRefreshesPerOrigin;
       ++i) {
    SCOPED_TRACE(Message() << "Iteration #" << (i + 1));
    EXPECT_TRUE(allocator().IsRefreshingSupported(GetSampleOrigin1()));

    base::MockCallback<PlusAddressRequestCallback> callback;
    EXPECT_CALL(callback, Run(PlusProfileOrError(kSampleProfile)));
    allocator().AllocatePlusAddress(
        GetSampleOrigin1(),
        PlusAddressAllocator::AllocationMode::kNewPlusAddress, callback.Get());
  }

  EXPECT_FALSE(allocator().IsRefreshingSupported(GetSampleOrigin1()));
  {
    base::MockCallback<PlusAddressRequestCallback> callback;
    EXPECT_CALL(callback, Run(kMaxRefreshesReachedError));
    allocator().AllocatePlusAddress(
        GetSampleOrigin1(),
        PlusAddressAllocator::AllocationMode::kNewPlusAddress, callback.Get());
  }

  // However, refreshing addresses on a different origin still works.
  EXPECT_TRUE(allocator().IsRefreshingSupported(GetSampleOrigin2()));
  {
    base::MockCallback<PlusAddressRequestCallback> callback;
    EXPECT_CALL(callback, Run(PlusProfileOrError(kSampleProfile)));
    allocator().AllocatePlusAddress(
        GetSampleOrigin2(),
        PlusAddressAllocator::AllocationMode::kNewPlusAddress, callback.Get());
  }
}

}  // namespace plus_addresses
