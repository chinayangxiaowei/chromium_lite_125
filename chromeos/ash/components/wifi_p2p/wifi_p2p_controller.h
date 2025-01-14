// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_WIFI_P2P_WIFI_P2P_CONTROLLER_H_
#define CHROMEOS_ASH_COMPONENTS_WIFI_P2P_WIFI_P2P_CONTROLLER_H_

#include "base/check.h"
#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/values.h"

namespace ash {

// Class for handling initialization and access to chromeos wifi_p2p controller.
// Exposes functions for following operations:
// 1. Create a p2p group
// 2. Destroy a p2p group
// 3. Connect to a p2p group
// 4. Disconnect from a p2p group
// 5. Fetch p2p group/client properties
// 6. Tag socket to a WiFi direct group network rules.
class COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_WIFI_P2P) WifiP2PController {
 public:
  // Sets the global instance. Must be called before any calls to Get().
  static void Initialize();

  // Destroys the global instance.
  static void Shutdown();

  // Gets the global instance. Initialize() must be called first.
  static WifiP2PController* Get();

  // Returns true if the global instance has been initialized.
  static bool IsInitialized();

  // Wifi direct group metadata includes: shill_id and frequency.
  struct WifiDirectConnectionMetadata {
    // Unique ID to identify the Wifi direct group.
    int shill_id;
    // The operating frequency of the Wifi direct group network.
    uint32_t frequency;
    // Unique ID to identify the network in Patchpanel.
    int network_id;
  };

  enum OperationResult {
    kSuccess,
    // Wifi direct is disallowed in platform per Manager.P2PAllowed.
    kNotAllowed,
    // Wifi direct operation is not supported in the platform.
    kNotSupported,
    // Creating Wifi direct interface is not possible with existing interfaces.
    kConcurrencyNotSupported,
    // The requested refruency is not supported.
    kFrequencyNotSupported,
    // Wifi direct group rejects the authentication attempt.
    kAuthFailure,
    // Didn't discover the Wifi direct group.
    kGroupNotFound,
    // Already connected to the Wifi direct group.
    kAlreadyConnected,
    // Wifi direct operation is already in progress.
    kOperationInProgress,
    // Invalid arguments.
    kInvalidArguments,
    // Wifi direct operation timed out.
    kTimeout,
    // Wifi direct operation response has an invalid result code.
    kInvalidResultCode,
    // Wifi direct group miss or has invalid properties.
    kInvalidGroupProperties,
    // Wifi direct operation failure.
    kOperationFailed,
    // Wifi direct operation failed due to DBus error.
    kDBusError,
    // Unknown error.
    kUnknownError,
  };

  // Return callback for the CreateWifiP2PGroup or ConnectToWifiP2PGroup
  // methods.
  using WifiP2PGroupCallback = base::OnceCallback<void(
      OperationResult result,
      std::optional<WifiDirectConnectionMetadata> metadata)>;

  // Create a Wifi P2P group with the given `ssid` and `passphrase`.
  void CreateWifiP2PGroup(const std::string& ssid,
                          const std::string& passphrase,
                          WifiP2PGroupCallback callback);

  // Connect to a Wifi P2P group with given `ssid` and `passphrase`. If
  // `frequency` is provided, the operation will fail if no group found at the
  // specified frequency. If it is omitted, the system will scan full supported
  // channels to find the group.
  void ConnectToWifiP2PGroup(const std::string& ssid,
                             const std::string& passphrase,
                             std::optional<uint32_t> frequency,
                             WifiP2PGroupCallback callback);

 private:
  WifiP2PController();
  WifiP2PController(const WifiP2PController&) = delete;
  WifiP2PController& operator=(const WifiP2PController&) = delete;
  ~WifiP2PController();

  void Init();

  void OnCreateOrConnectP2PGroupSuccess(bool create_group,
                                        WifiP2PGroupCallback callback,
                                        base::Value::Dict result);

  void OnCreateOrConnectP2PGroupFailure(WifiP2PGroupCallback callback,
                                        const std::string& error_name,
                                        const std::string& error_message);
  void GetP2PGroupMetadata(int shill_id,
                           bool is_owner,
                           WifiP2PGroupCallback callback,
                           std::optional<base::Value::Dict> properties);

  // Callback when set shill manager property operation failed.
  void OnSetManagerPropertyFailure(const std::string& property_name,
                                   const std::string& error_name,
                                   const std::string& error_message);

  base::WeakPtrFactory<WifiP2PController> weak_ptr_factory_{this};
};

}  // namespace ash

#endif  // CHROMEOS_ASH_COMPONENTS_WIFI_P2P_WIFI_P2P_CONTROLLER_H_
