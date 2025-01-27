// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_mac.h"

#import <IOBluetooth/IOBluetooth.h>

#include "base/memory/raw_ptr.h"
#import "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#import "device/bluetooth/test/test_bluetooth_adapter_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

using ::testing::Return;

class BluetoothAdapterMacTest : public testing::Test {
 public:
  BluetoothAdapterMacTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        adapter_(new BluetoothAdapterMac()),
        adapter_mac_(static_cast<BluetoothAdapterMac*>(adapter_.get())),
        observer_(adapter_) {
    adapter_mac_->InitForTest(ui_task_runner_);
  }

  void TearDown() override { task_environment_.RunUntilIdle(); }

  // Helper methods for setup and access to BluetoothAdapterMacTest's
  // members.
  void PollAdapter() { adapter_mac_->PollAdapter(); }

  void SetHostControllerPowerFunction(bool powered) {
    adapter_mac_->SetHostControllerStateFunctionForTesting(
        base::BindLambdaForTesting([powered] {
          BluetoothAdapterMac::HostControllerState state;
          state.classic_powered = powered;
          return state;
        }));
  }

  void AddClassicDevice(const std::string& device_address,
                        BluetoothDevice::UUIDSet uuids) {
    auto device = std::make_unique<testing::NiceMock<MockBluetoothDevice>>(
        adapter_mac_,
        /*bluetooth_class=*/0, "device-name", device_address,
        /*initially_paired=*/true,
        /*connected=*/false);
    EXPECT_CALL(*device, GetUUIDs).WillRepeatedly(Return(uuids));
    adapter_mac_->ClassicDeviceAdded(std::move(device));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<BluetoothAdapter> adapter_;
  raw_ptr<BluetoothAdapterMac> adapter_mac_;
  TestBluetoothAdapterObserver observer_;
};

TEST_F(BluetoothAdapterMacTest, Poll) {
  PollAdapter();
  EXPECT_TRUE(ui_task_runner_->HasPendingTask());
}

TEST_F(BluetoothAdapterMacTest, PollAndChangePower) {
  // By default the adapter is powered off, check that this expectation matches
  // reality.
  EXPECT_FALSE(adapter_mac_->IsPowered());
  EXPECT_EQ(0, observer_.powered_changed_count());

  SetHostControllerPowerFunction(true);
  PollAdapter();
  EXPECT_TRUE(ui_task_runner_->HasPendingTask());
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, observer_.powered_changed_count());
  EXPECT_TRUE(observer_.last_powered());
  EXPECT_TRUE(adapter_mac_->IsPowered());

  SetHostControllerPowerFunction(false);
  PollAdapter();
  EXPECT_TRUE(ui_task_runner_->HasPendingTask());
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(2, observer_.powered_changed_count());
  EXPECT_FALSE(observer_.last_powered());
  EXPECT_FALSE(adapter_mac_->IsPowered());
}

TEST_F(BluetoothAdapterMacTest, ClassicDeviceAddedAndChanged) {
  // Simulate a paired Bluetooth Classic device with one service UUID.
  std::string device_address = "AA:BB:CC:DD:EE:FF";
  BluetoothDevice::UUIDSet uuids;
  uuids.insert(BluetoothUUID("110b"));
  AddClassicDevice(device_address, uuids);
  EXPECT_EQ(1, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_changed_count());
  EXPECT_EQ(observer_.last_device_address(), device_address);
  observer_.Reset();

  // Adding the same device again does not notify observers.
  AddClassicDevice(device_address, uuids);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_changed_count());
  observer_.Reset();

  // Update the device by adding a second service UUID.
  uuids.insert(BluetoothUUID("110c"));
  AddClassicDevice(device_address, uuids);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(1, observer_.device_changed_count());
  EXPECT_EQ(observer_.last_device_address(), device_address);
}

}  // namespace device
