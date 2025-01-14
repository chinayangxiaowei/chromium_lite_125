// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_SHARING_NEARBY_PLATFORM_BLE_V2_GATT_SERVER_H_
#define CHROME_SERVICES_SHARING_NEARBY_PLATFORM_BLE_V2_GATT_SERVER_H_

#include "base/containers/flat_map.h"
#include "chrome/services/sharing/nearby/platform/bluetooth_adapter.h"
#include "device/bluetooth/public/mojom/adapter.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/shared_remote.h"
#include "third_party/nearby/src/internal/platform/implementation/ble_v2.h"

namespace nearby::chrome {

class BleV2GattServer : public ::nearby::api::ble_v2::GattServer,
                        public bluetooth::mojom::GattServiceObserver {
 public:
  explicit BleV2GattServer(
      const mojo::SharedRemote<bluetooth::mojom::Adapter>& adapter);
  ~BleV2GattServer() override;

  BleV2GattServer(const BleV2GattServer&) = delete;
  BleV2GattServer& operator=(const BleV2GattServer&) = delete;

  // nearby::api::ble_v2::GattServer:
  BluetoothAdapter& GetBlePeripheral() override;
  std::optional<api::ble_v2::GattCharacteristic> CreateCharacteristic(
      const Uuid& service_uuid,
      const Uuid& characteristic_uuid,
      api::ble_v2::GattCharacteristic::Permission permission,
      api::ble_v2::GattCharacteristic::Property property) override;
  bool UpdateCharacteristic(
      const api::ble_v2::GattCharacteristic& characteristic,
      const nearby::ByteArray& value) override;
  absl::Status NotifyCharacteristicChanged(
      const api::ble_v2::GattCharacteristic& characteristic,
      bool confirm,
      const nearby::ByteArray& new_value) override;
  void Stop() override;

 private:
  struct GattService {
    GattService();
    ~GattService();

    mojo::Remote<bluetooth::mojom::GattService> gatt_service_remote;
    base::flat_map<Uuid, api::ble_v2::GattCharacteristic>
        characteristic_uuid_to_characteristic_map;

    // Characteristic UUID to value map. The value is set
    // in `UpdateCharacteristic()`, and this class is responsible for storing
    // the value of a GATT characteristic. See documentation in
    // `UpdateCharacteristic()`.
    base::flat_map<Uuid, nearby::ByteArray> characteristic_uuid_to_value_map;
  };

  // bluetooth::mojom::GattServiceObserver:
  void OnLocalCharacteristicRead(
      bluetooth::mojom::DeviceInfoPtr remote_device,
      const device::BluetoothUUID& characteristic_uuid,
      const device::BluetoothUUID& service_uuid,
      uint32_t offset,
      OnLocalCharacteristicReadCallback callback) override;

  std::unique_ptr<BluetoothAdapter> bluetooth_adapter_;
  base::flat_map<Uuid, std::unique_ptr<GattService>> uuid_to_gatt_service_map_;
  mojo::SharedRemote<bluetooth::mojom::Adapter> adapter_remote_;
  mojo::Receiver<bluetooth::mojom::GattServiceObserver> gatt_service_observer_{
      this};
};

}  // namespace nearby::chrome

#endif  // CHROME_SERVICES_SHARING_NEARBY_PLATFORM_BLE_V2_GATT_SERVER_H_
