// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/ash/components/tether/tether_availability_operation.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "base/test/task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "chromeos/ash/components/multidevice/remote_device_test_util.h"
#include "chromeos/ash/components/tether/fake_connection_preserver.h"
#include "chromeos/ash/components/tether/host_scan_device_prioritizer.h"
#include "chromeos/ash/components/tether/message_wrapper.h"
#include "chromeos/ash/components/tether/mock_tether_host_response_recorder.h"
#include "chromeos/ash/components/tether/proto/tether.pb.h"
#include "chromeos/ash/components/tether/proto_test_util.h"
#include "chromeos/ash/services/device_sync/public/cpp/fake_device_sync_client.h"
#include "chromeos/ash/services/secure_channel/public/cpp/client/fake_client_channel.h"
#include "chromeos/ash/services/secure_channel/public/cpp/client/fake_connection_attempt.h"
#include "chromeos/ash/services/secure_channel/public/cpp/client/fake_secure_channel_client.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::StrictMock;

namespace ash {

namespace tether {

namespace {

DeviceStatus CreateFakeDeviceStatus() {
  return CreateTestDeviceStatus("Google Fi", 75 /* battery_percentage */,
                                4 /* connection_strength */);
}

}  // namespace

class TetherAvailabilityOperationTest : public testing::Test {
 public:
  TetherAvailabilityOperationTest(const TetherAvailabilityOperationTest&) =
      delete;
  TetherAvailabilityOperationTest& operator=(
      const TetherAvailabilityOperationTest&) = delete;

 protected:
  TetherAvailabilityOperationTest()
      : local_device_(multidevice::RemoteDeviceRefBuilder()
                          .SetPublicKey("local device")
                          .Build()),
        remote_device_(multidevice::CreateRemoteDeviceRefListForTest(1)[0]) {}

  void SetUp() override {
    fake_device_sync_client_.set_local_device_metadata(local_device_);

    operation_ = ConstructOperation();
    operation_->Initialize();

    ConnectAuthenticatedChannelForDevice(remote_device_);
  }

  multidevice::RemoteDeviceRef GetOperationRemoteDevice(
      TetherAvailabilityOperation* operation) const {
    return operation->remote_device();
  }

  std::unique_ptr<TetherAvailabilityOperation> ConstructOperation() {
    auto connection_attempt =
        std::make_unique<secure_channel::FakeConnectionAttempt>();
    connection_attempt_ = connection_attempt.get();
    fake_secure_channel_client_.set_next_listen_connection_attempt(
        remote_device_, local_device_, std::move(connection_attempt));

    auto operation = std::make_unique<TetherAvailabilityOperation>(
        remote_device_,
        base::BindOnce(&TetherAvailabilityOperationTest::OnResponse,
                       weak_ptr_factory_.GetWeakPtr()),
        &fake_device_sync_client_, &fake_secure_channel_client_,
        &mock_tether_host_response_recorder_, &fake_connection_preserver_);

    test_clock_.SetNow(base::Time::UnixEpoch());
    test_task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
    operation->SetTestDoubles(&test_clock_, test_task_runner_);

    return operation;
  }

  void ConnectAuthenticatedChannelForDevice(
      multidevice::RemoteDeviceRef remote_device) {
    auto fake_client_channel =
        std::make_unique<secure_channel::FakeClientChannel>();
    connection_attempt_->NotifyConnection(std::move(fake_client_channel));
  }

  void OnResponse(std::optional<ScannedDeviceResult> result) {
    received_result_ = result;
  }

  std::optional<ScannedDeviceResult> received_result_;
  const multidevice::RemoteDeviceRef local_device_;
  const multidevice::RemoteDeviceRef remote_device_;

  raw_ptr<secure_channel::FakeConnectionAttempt, DanglingUntriaged>
      connection_attempt_;
  device_sync::FakeDeviceSyncClient fake_device_sync_client_;
  secure_channel::FakeSecureChannelClient fake_secure_channel_client_;
  StrictMock<MockTetherHostResponseRecorder>
      mock_tether_host_response_recorder_;
  FakeConnectionPreserver fake_connection_preserver_;

  base::test::TaskEnvironment task_environment_;
  base::SimpleTestClock test_clock_;
  scoped_refptr<base::TestSimpleTaskRunner> test_task_runner_;
  base::HistogramTester histogram_tester_;

  std::unique_ptr<TetherAvailabilityOperation> operation_;
  base::WeakPtrFactory<TetherAvailabilityOperationTest> weak_ptr_factory_{this};
};

TEST_F(TetherAvailabilityOperationTest,
       SendsTetherAvailabilityRequestOnceAuthenticated) {
  std::unique_ptr<TetherAvailabilityOperation> operation = ConstructOperation();
  operation->Initialize();

  // Create the client channel to the remote device.
  auto fake_client_channel =
      std::make_unique<secure_channel::FakeClientChannel>();

  // No requests as a result of creating the client channel.
  auto& sent_messages = fake_client_channel->sent_messages();
  EXPECT_EQ(0u, sent_messages.size());

  // Connect and authenticate the client channel.
  connection_attempt_->NotifyConnection(std::move(fake_client_channel));

  // Verify the TetherAvailabilityRequest message is sent.
  auto message_wrapper =
      std::make_unique<MessageWrapper>(TetherAvailabilityRequest());
  std::string expected_payload = message_wrapper->ToRawMessage();
  EXPECT_EQ(1u, sent_messages.size());
  EXPECT_EQ(expected_payload, sent_messages[0].first);
}

TEST_F(TetherAvailabilityOperationTest, RecordsResponseDuration) {
  static constexpr base::TimeDelta kTetherAvailabilityResponseTime =
      base::Seconds(3);

  // Advance the clock in order to verify a non-zero response duration is
  // recorded and verified (below).
  test_clock_.Advance(kTetherAvailabilityResponseTime);

  std::unique_ptr<MessageWrapper> message(
      new MessageWrapper(TetherAvailabilityResponse()));
  operation_->OnMessageReceived(std::move(message));

  histogram_tester_.ExpectTimeBucketCount(
      "InstantTethering.Performance.TetherAvailabilityResponseDuration",
      kTetherAvailabilityResponseTime, 1);
}

// Tests that the TetherAvailabilityOperation does not record a potential tether
// connection after receiving an error response.
TEST_F(TetherAvailabilityOperationTest, ErrorResponses) {
  const TetherAvailabilityResponse_ResponseCode kErrorResponseCodes[] = {
      TetherAvailabilityResponse_ResponseCode_UNKNOWN_ERROR,
      TetherAvailabilityResponse_ResponseCode_NO_RECEPTION,
      TetherAvailabilityResponse_ResponseCode_NO_SIM_CARD};

  for (auto response_code : kErrorResponseCodes) {
    // No response should be recorded.
    EXPECT_CALL(mock_tether_host_response_recorder_,
                RecordSuccessfulTetherAvailabilityResponse(_))
        .Times(0);

    // Respond with the error code.
    TetherAvailabilityResponse response;
    response.set_response_code(response_code);
    std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
    operation_->OnMessageReceived(std::move(message));

    // Connection is not preserved.
    bool connection_preserved =
        !fake_connection_preserver_
             .last_requested_preserved_connection_device_id()
             .empty();
    EXPECT_FALSE(connection_preserved);
    EXPECT_FALSE(received_result_.has_value());
  }
}

// Tests that the observer is notified of the list of devices whose
// notifications are disabled each time a new response is received.
TEST_F(TetherAvailabilityOperationTest, NotificationsDisabled) {
  // No response should be recorded.
  EXPECT_CALL(mock_tether_host_response_recorder_,
              RecordSuccessfulTetherAvailabilityResponse(_))
      .Times(0);

  // Respond with the error code.
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode_NOTIFICATIONS_DISABLED_LEGACY);
  std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
  operation_->OnMessageReceived(std::move(message));

  test_task_runner_->RunUntilIdle();

  // Connection is not preserved.
  bool connection_preserved =
      !fake_connection_preserver_
           .last_requested_preserved_connection_device_id()
           .empty();
  EXPECT_FALSE(connection_preserved);
  EXPECT_EQ(received_result_.value().error(),
            ScannedDeviceInfoError::kNotificationsDisabled);
}

TEST_F(TetherAvailabilityOperationTest,
       NotificationsDisabledWithNotificationChannel) {
  // No response should be recorded.
  EXPECT_CALL(mock_tether_host_response_recorder_,
              RecordSuccessfulTetherAvailabilityResponse(_))
      .Times(0);

  // Respond with the error code.
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode_NOTIFICATIONS_DISABLED_WITH_NOTIFICATION_CHANNEL);
  std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
  operation_->OnMessageReceived(std::move(message));

  test_task_runner_->RunUntilIdle();

  // Connection is not preserved.
  bool connection_preserved =
      !fake_connection_preserver_
           .last_requested_preserved_connection_device_id()
           .empty();
  EXPECT_FALSE(connection_preserved);
  EXPECT_EQ(received_result_.value().error(),
            ScannedDeviceInfoError::kNotificationsDisabled);
}

TEST_F(TetherAvailabilityOperationTest, TetherAvailable) {
  // The scanned device is recorded.
  EXPECT_CALL(mock_tether_host_response_recorder_,
              RecordSuccessfulTetherAvailabilityResponse(remote_device_));

  // The observer is notified of the scanned device.
  DeviceStatus device_status = CreateFakeDeviceStatus();
  ScannedDeviceInfo scanned_device(remote_device_, device_status,
                                   false /* setup_required */);

  // Respond with TETHER_AVAILABLE response code and the device info and status.
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode_TETHER_AVAILABLE);
  response.mutable_device_status()->CopyFrom(device_status);
  std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
  operation_->OnMessageReceived(std::move(message));

  test_task_runner_->RunUntilIdle();

  // Connection is preserved.
  EXPECT_EQ(remote_device_.GetDeviceId(),
            fake_connection_preserver_
                .last_requested_preserved_connection_device_id());

  EXPECT_TRUE(received_result_.has_value());
  EXPECT_EQ(received_result_.value().value(), scanned_device);
}

TEST_F(TetherAvailabilityOperationTest, LastProvisioningFailed) {
  // The scanned device is recorded.
  EXPECT_CALL(mock_tether_host_response_recorder_,
              RecordSuccessfulTetherAvailabilityResponse(remote_device_));

  // The observer is notified of the scanned device.
  DeviceStatus device_status = CreateFakeDeviceStatus();
  ScannedDeviceInfo scanned_device(remote_device_, device_status,
                                   false /* setup_required */);
  std::vector<ScannedDeviceInfo> scanned_devices({scanned_device});

  // Respond with TETHER_AVAILABLE response code and the device info and status.
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode_LAST_PROVISIONING_FAILED);
  response.mutable_device_status()->CopyFrom(device_status);
  std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
  operation_->OnMessageReceived(std::move(message));

  // Connection is preserved.
  EXPECT_EQ(remote_device_.GetDeviceId(),
            fake_connection_preserver_
                .last_requested_preserved_connection_device_id());

  test_task_runner_->RunUntilIdle();

  EXPECT_EQ(scanned_device, received_result_.value().value());
}

TEST_F(TetherAvailabilityOperationTest, SetupRequired) {
  // The scanned device is recorded.
  EXPECT_CALL(mock_tether_host_response_recorder_,
              RecordSuccessfulTetherAvailabilityResponse(remote_device_));

  // The observer is notified that the scanned device has the |setup_required|
  // flag set.
  DeviceStatus device_status = CreateFakeDeviceStatus();
  ScannedDeviceInfo scanned_device(remote_device_, device_status,
                                   true /* setup_required */);
  std::vector<ScannedDeviceInfo> scanned_devices({scanned_device});

  // Respond with SETUP_NEEDED response code and the device info and status.
  TetherAvailabilityResponse response;
  response.set_response_code(
      TetherAvailabilityResponse_ResponseCode_SETUP_NEEDED);
  response.mutable_device_status()->CopyFrom(device_status);
  std::unique_ptr<MessageWrapper> message(new MessageWrapper(response));
  operation_->OnMessageReceived(std::move(message));

  test_task_runner_->RunUntilIdle();

  // Connection is preserved.
  EXPECT_EQ(remote_device_.GetDeviceId(),
            fake_connection_preserver_
                .last_requested_preserved_connection_device_id());
  EXPECT_EQ(scanned_device, received_result_.value().value());
}

}  // namespace tether

}  // namespace ash
