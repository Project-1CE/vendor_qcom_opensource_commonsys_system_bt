/******************************************************************************
 *
 *  Copyright (C) 2016 Google Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************/

#define LOG_TAG "bt_btif_ble_advertiser"

#include <hardware/bluetooth.h>
#include <hardware/bt_gatt.h>

#include <base/bind.h>
#include <base/callback.h>
#include <vector>

#include "ble_advertiser.h"
#include "bta_closure_api.h"
#include "btif_common.h"

using base::Bind;
using base::Owned;
using std::vector;

namespace {

template <typename T>
class OwnedArrayWrapper {
 public:
  explicit OwnedArrayWrapper(T* o) : ptr_(o) {}
  ~OwnedArrayWrapper() { delete[] ptr_; }
  T* get() const { return ptr_; }
  OwnedArrayWrapper(OwnedArrayWrapper&& other) noexcept {
    ptr_ = other.ptr_;
    other.ptr_ = NULL;
  }

 private:
  mutable T* ptr_;
};

template <typename T>
T* Unwrap(const OwnedArrayWrapper<T>& o) {
  return o.get();
}

template <typename T>
static inline OwnedArrayWrapper<T> OwnedArray(T* o) {
  return OwnedArrayWrapper<T>(o);
}

void parseParams(tBTM_BLE_ADV_PARAMS* p_params,
                 const AdvertiseParameters& params) {
  p_params->advertising_event_properties = params.advertising_event_properties;
  p_params->adv_int_min = params.min_interval;
  p_params->adv_int_max = params.max_interval;
  p_params->channel_map = params.channel_map;
  p_params->adv_filter_policy = 0;
  p_params->tx_power = params.tx_power;
  p_params->primary_advertising_phy = params.primary_advertising_phy;
  p_params->secondary_advertising_phy = params.secondary_advertising_phy;
  p_params->scan_request_notification_enable =
      params.scan_request_notification_enable;
  p_params->own_address_type = params.own_address_type;
}

void parsePeriodicParams(tBLE_PERIODIC_ADV_PARAMS* p_periodic_params,
                         PeriodicAdvertisingParameters periodic_params) {
  p_periodic_params->enable = periodic_params.enable;
  p_periodic_params->include_adi = periodic_params.include_adi;
  p_periodic_params->min_interval = periodic_params.min_interval;
  p_periodic_params->max_interval = periodic_params.max_interval;
  p_periodic_params->periodic_advertising_properties =
      periodic_params.periodic_advertising_properties;
}

#if (BLE_ISO_IF_SUPPORTED == TRUE)
void parseCreateBIGParams(tBLE_CREATE_BIG_PARAMS* p_create_big_params,
                          CreateBIGParameters create_big_params) {
  p_create_big_params->adv_handle = create_big_params.adv_handle;
  p_create_big_params->num_bis = create_big_params.num_bis;
  p_create_big_params->sdu_int = create_big_params.sdu_int;
  p_create_big_params->max_sdu = create_big_params.max_sdu;
  p_create_big_params->max_transport_latency = create_big_params.max_transport_latency;
  p_create_big_params->rtn = create_big_params.rtn;
  p_create_big_params->phy = create_big_params.phy;
  p_create_big_params->packing = create_big_params.packing;
  p_create_big_params->framing = create_big_params.framing;
  p_create_big_params->encryption = create_big_params.encryption;
  p_create_big_params->broadcast_code = create_big_params.broadcast_code;
}
#endif /* BLE_ISO_IF_SUPPORTED == TRUE */

class BleAdvertiserInterfaceImpl : public BleAdvertiserInterface {
  ~BleAdvertiserInterfaceImpl(){};

  void RegisterAdvertiserCb(IdStatusCallback cb, uint8_t advertiser_id,
                            uint8_t status) {
    LOG(INFO) << __func__ << " status: " << +status
              << " , adveriser_id: " << +advertiser_id;
    do_in_jni_thread(Bind(cb, advertiser_id, status));
  }

  void RegisterAdvertiser(IdStatusCallback cb) override {
    do_in_bta_thread(
        FROM_HERE, Bind(&BleAdvertisingManager::RegisterAdvertiser,
                        BleAdvertisingManager::Get(),
                        Bind(&BleAdvertiserInterfaceImpl::RegisterAdvertiserCb,
                             base::Unretained(this), cb)));
  }

  void Unregister(uint8_t advertiser_id) override {
    do_in_bta_thread(
        FROM_HERE,
        Bind(
            [](uint8_t advertiser_id) {
              if (!BleAdvertisingManager::IsInitialized()) {
                LOG(WARNING) << "Stack already shutdown";
                return;
              }
              BleAdvertisingManager::Get()->Unregister(advertiser_id);
            },
            advertiser_id));
  }

  void GetOwnAddress(uint8_t advertiser_id, GetAddressCallback cb) override {
    if (!BleAdvertisingManager::IsInitialized()) return;
    do_in_bta_thread(FROM_HERE,
                     Bind(&BleAdvertisingManager::GetOwnAddress,
                          BleAdvertisingManager::Get(), advertiser_id,
                          jni_thread_wrapper(FROM_HERE, cb)));
  }

  void SetParameters(uint8_t advertiser_id, AdvertiseParameters params,
                     ParametersCallback cb) override {
    VLOG(1) << __func__;

    if (!BleAdvertisingManager::IsInitialized()) return;
    tBTM_BLE_ADV_PARAMS* p_params = new tBTM_BLE_ADV_PARAMS;
    parseParams(p_params, params);

    do_in_bta_thread(FROM_HERE, Bind(&BleAdvertisingManager::SetParameters,
                                     BleAdvertisingManager::Get(),
                                     advertiser_id, base::Owned(p_params),
                                     jni_thread_wrapper(FROM_HERE, cb)));
  }

  void SetData(int advertiser_id, bool set_scan_rsp, std::vector<uint8_t> data,
               std::vector<uint8_t> data_enc, StatusCallback cb) override {
    if (!BleAdvertisingManager::IsInitialized()) return;
    do_in_bta_thread(
        FROM_HERE,
        Bind(&BleAdvertisingManager::SetData, BleAdvertisingManager::Get(),
             advertiser_id, set_scan_rsp, std::move(data), std::move(data_enc),
             jni_thread_wrapper(FROM_HERE, cb)));
  }

  void Enable(uint8_t advertiser_id, bool enable, StatusCallback cb,
              uint16_t duration, uint8_t maxExtAdvEvents,
              StatusCallback timeout_cb) override {
    VLOG(1) << __func__ << " advertiser_id: " << +advertiser_id
            << " ,enable: " << enable;

    if (!BleAdvertisingManager::IsInitialized()) return;
    do_in_bta_thread(
        FROM_HERE,
        Bind(&BleAdvertisingManager::Enable, BleAdvertisingManager::Get(),
             advertiser_id, enable, jni_thread_wrapper(FROM_HERE, cb), duration,
             maxExtAdvEvents, jni_thread_wrapper(FROM_HERE, timeout_cb)));
  }

  void StartAdvertising(uint8_t advertiser_id, StatusCallback cb,
                        AdvertiseParameters params,
                        std::vector<uint8_t> advertise_data,
                        std::vector<uint8_t> scan_response_data, int timeout_s,
                        MultiAdvCb timeout_cb) override {
    VLOG(1) << __func__;

    if (!BleAdvertisingManager::IsInitialized()) return;
    tBTM_BLE_ADV_PARAMS* p_params = new tBTM_BLE_ADV_PARAMS;
    parseParams(p_params, params);

    do_in_bta_thread(
        FROM_HERE,
        Bind(&BleAdvertisingManager::StartAdvertising,
             BleAdvertisingManager::Get(), advertiser_id,
             jni_thread_wrapper(FROM_HERE, cb), base::Owned(p_params),
             std::move(advertise_data), std::move(scan_response_data),
             timeout_s * 100, jni_thread_wrapper(FROM_HERE, timeout_cb)));
  }
  
  void StartAdvertisingSet(int reg_id, IdTxPowerStatusCallback cb,
                           AdvertiseParameters params,
                           std::vector<uint8_t> advertise_data,
                           std::vector<uint8_t> advertise_data_enc,
                           std::vector<uint8_t> scan_response_data,
                           std::vector<uint8_t> scan_response_data_enc,
                           PeriodicAdvertisingParameters periodic_params,
                           std::vector<uint8_t> periodic_data,
                           std::vector<uint8_t> periodic_data_enc,
                           uint16_t duration, uint8_t maxExtAdvEvents,
                           std::vector<uint8_t> enc_key_value,
                           IdStatusCallback timeout_cb) override {
    VLOG(1) << __func__;

    if (!BleAdvertisingManager::IsInitialized()) return;
    tBTM_BLE_ADV_PARAMS* p_params = new tBTM_BLE_ADV_PARAMS;
    parseParams(p_params, params);

    tBLE_PERIODIC_ADV_PARAMS* p_periodic_params = new tBLE_PERIODIC_ADV_PARAMS;
    parsePeriodicParams(p_periodic_params, periodic_params);

    do_in_bta_thread(
        FROM_HERE,
        Bind(&BleAdvertisingManager::StartAdvertisingSet,
             BleAdvertisingManager::Get(), jni_thread_wrapper(FROM_HERE, cb),
             base::Owned(p_params), std::move(advertise_data),
             std::move(advertise_data_enc), std::move(scan_response_data),
             std::move(scan_response_data_enc), base::Owned(p_periodic_params),
             std::move(periodic_data), std::move(periodic_data_enc),
             duration, maxExtAdvEvents, std::move(enc_key_value),
             jni_thread_wrapper(FROM_HERE, timeout_cb)));

    return;
  }

  void SetPeriodicAdvertisingParameters(
      int advertiser_id, PeriodicAdvertisingParameters periodic_params,
      StatusCallback cb) override {
    VLOG(1) << __func__ << " advertiser_id: " << +advertiser_id;

    if (!BleAdvertisingManager::IsInitialized()) return;
    tBLE_PERIODIC_ADV_PARAMS* p_periodic_params = new tBLE_PERIODIC_ADV_PARAMS;
    parsePeriodicParams(p_periodic_params, periodic_params);

    do_in_bta_thread(
        FROM_HERE,
        Bind(&BleAdvertisingManager::SetPeriodicAdvertisingParameters,
             BleAdvertisingManager::Get(), advertiser_id,
             base::Owned(p_periodic_params),
             jni_thread_wrapper(FROM_HERE, cb)));
  }

  void SetPeriodicAdvertisingData(int advertiser_id, std::vector<uint8_t> data,
                                  std::vector<uint8_t> data_enc,
                                  StatusCallback cb) override {
    VLOG(1) << __func__ << " advertiser_id: " << +advertiser_id;

    if (!BleAdvertisingManager::IsInitialized()) return;
    do_in_bta_thread(FROM_HERE,
                     Bind(&BleAdvertisingManager::SetPeriodicAdvertisingData,
                          BleAdvertisingManager::Get(), advertiser_id,
                          std::move(data), std::move(data_enc),
                          jni_thread_wrapper(FROM_HERE, cb)));
  }

  void SetPeriodicAdvertisingEnable(int advertiser_id, bool enable,
                                    bool include_adi,
                                    StatusCallback cb) override {
    VLOG(1) << __func__ << " advertiser_id: " << +advertiser_id
            << " ,enable: " << enable;

    if (!BleAdvertisingManager::IsInitialized()) return;
    do_in_bta_thread(FROM_HERE,
                     Bind(&BleAdvertisingManager::SetPeriodicAdvertisingEnable,
                          BleAdvertisingManager::Get(), advertiser_id, enable,
                          jni_thread_wrapper(FROM_HERE, cb)));
  }

  void RegisterCallbacks(AdvertisingCallbacks* callbacks) {
    // For GD only
  }

#if (BLE_ISO_IF_SUPPORTED == TRUE)
  void CreateBIG(int advertiser_id, CreateBIGParameters create_big_params,
                 CreateBIGCallback cb) override {
    VLOG(1) << __func__ << " advertiser_id: " << +advertiser_id;

    if (!BleAdvertisingManager::IsInitialized()) return;
    tBLE_CREATE_BIG_PARAMS* p_create_big_params = new tBLE_CREATE_BIG_PARAMS;
    parseCreateBIGParams(p_create_big_params, create_big_params);

    do_in_bta_thread(FROM_HERE,
                     Bind(&BleAdvertisingManager::CreateBIG,
                          BleAdvertisingManager::Get(), advertiser_id,
                          base::Owned(p_create_big_params),
                          jni_thread_wrapper(FROM_HERE, cb)));
  }

  void TerminateBIG(int advertiser_id, int big_handle, int reason,
                    TerminateBIGCallback cb) override {
    VLOG(1) << __func__ << "big_handle: " << +big_handle;

    if (!BleAdvertisingManager::IsInitialized()) return;

    do_in_bta_thread(FROM_HERE,
                     Bind(&BleAdvertisingManager::TerminateBIG,
                          BleAdvertisingManager::Get(), advertiser_id,
                          big_handle, reason,
                          jni_thread_wrapper(FROM_HERE, cb)));
  }
#endif /* BLE_ISO_IF_SUPPORTED == TRUE */
};

BleAdvertiserInterface* btLeAdvertiserInstance = nullptr;

}  // namespace

BleAdvertiserInterface* get_ble_advertiser_instance() {
  if (btLeAdvertiserInstance == nullptr)
    btLeAdvertiserInstance = new BleAdvertiserInterfaceImpl();

  return btLeAdvertiserInstance;
}
