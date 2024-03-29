/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci"

#include "hci_packet_parser.h"

#include <base/logging.h>

#include "buffer_allocator.h"
#include "hci_layer.h"
#include "hcimsgs.h"
#include "osi/include/log.h"

static const command_opcode_t NO_OPCODE_CHECKING = 0;

static const allocator_t* buffer_allocator;

static uint8_t* read_command_complete_header(BT_HDR* response,
                                             command_opcode_t expected_opcode,
                                             size_t minimum_bytes_after);

static void parse_generic_command_complete(BT_HDR* response) {
  if (response) {
    read_command_complete_header(response, NO_OPCODE_CHECKING,
                                 0 /* bytes after */);
    buffer_allocator->free(response);
  }
}

static void parse_read_buffer_size_response(BT_HDR* response,
                                            uint16_t* data_size_ptr,
                                            uint16_t* acl_buffer_count_ptr) {
  uint8_t* stream = read_command_complete_header(response, HCI_READ_BUFFER_SIZE,
                                                 5 /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_UINT16(*data_size_ptr, stream);
  STREAM_SKIP_UINT8(stream);  // skip the sco packet length
  STREAM_TO_UINT16(*acl_buffer_count_ptr, stream);

  buffer_allocator->free(response);
}

static void parse_read_local_version_info_response(BT_HDR* response,
                                                   bt_version_t* bt_version) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_READ_LOCAL_VERSION_INFO, 8 /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_UINT8(bt_version->hci_version, stream);
  STREAM_TO_UINT16(bt_version->hci_revision, stream);
  STREAM_TO_UINT8(bt_version->lmp_version, stream);
  STREAM_TO_UINT16(bt_version->manufacturer, stream);
  STREAM_TO_UINT16(bt_version->lmp_subversion, stream);

  buffer_allocator->free(response);
}

static void parse_read_local_supported_codecs_response(
    BT_HDR* response,
    uint8_t* number_of_std_supported_codecs,
    uint8_t* std_supported_codecs, uint8_t* std_codec_tx,
    uint8_t* number_of_vs_supported_codecs,
    uint32_t* vs_supported_codecs, uint8_t* vs_codec_tx) {

  uint8_t* stream = NULL;
  if (std_codec_tx) {
    stream = read_command_complete_header(
                    response, HCI_READ_LOCAL_SUPPORTED_CODECS_V2,
                    0 /* bytes after */);
  } else {
    stream = read_command_complete_header(
                    response, HCI_READ_LOCAL_SUPPORTED_CODECS,
                    0 /* bytes after */);
  }

  if (stream) {
    STREAM_TO_UINT8(*number_of_std_supported_codecs, stream);
    for (uint8_t i = 0; i < *number_of_std_supported_codecs; i++) {
      STREAM_TO_UINT8(*std_supported_codecs, stream);
      std_supported_codecs++;
      if (std_codec_tx) {
        STREAM_TO_UINT8(*std_codec_tx, stream);
        std_codec_tx++;
      }
    }
    STREAM_TO_UINT8(*number_of_vs_supported_codecs, stream);
    for (uint8_t i = 0; i < *number_of_vs_supported_codecs; i++) {
      STREAM_TO_UINT32(*vs_supported_codecs, stream);
      vs_supported_codecs++;
      if (vs_codec_tx) {
        STREAM_TO_UINT8(*vs_codec_tx, stream);
        vs_codec_tx++;
      }
    }
  }

  buffer_allocator->free(response);
}

static void parse_ble_read_buffer_size_response(
    BT_HDR* response,
    uint16_t* le_acl_data_packet_length,
    uint8_t* total_num_le_acl_data_packets,
    uint16_t* iso_data_packet_length,
    uint8_t* total_num_iso_data_packets) {

  uint8_t* stream = NULL;
  if (iso_data_packet_length) {
    stream = read_command_complete_header(response, HCI_BLE_READ_BUFFER_SIZE_V2,
                                          0 /* bytes after */);
  } else {
    stream = read_command_complete_header(response, HCI_BLE_READ_BUFFER_SIZE,
                                          0 /* bytes after */);
  }

  if (stream) {
    STREAM_TO_UINT16(*le_acl_data_packet_length, stream);
    STREAM_TO_UINT8(*total_num_le_acl_data_packets, stream);
    if (iso_data_packet_length && total_num_iso_data_packets) {
      STREAM_TO_UINT16(*iso_data_packet_length, stream);
      STREAM_TO_UINT8(*total_num_iso_data_packets, stream);
    }
  }

  buffer_allocator->free(response);
}

static void parse_read_scrambling_supported_freqs_response(
    BT_HDR* response, uint8_t* number_of_scrambling_supported_freqs,
    uint8_t* scrambling_supported_freqs) {
  uint8_t sub_opcode;
  uint8_t* stream = read_command_complete_header(
      response, HCI_VSC_SPLIT_A2DP_OPCODE, 0 /* bytes after */);
  if (stream) {
    STREAM_TO_UINT8(sub_opcode, stream);
  }
  if (stream && sub_opcode == VS_QHCI_GET_SCRAMBLING_FREQS) {
    STREAM_TO_UINT8(*number_of_scrambling_supported_freqs, stream);
    for (uint8_t i = 0; i < *number_of_scrambling_supported_freqs; i++) {
      STREAM_TO_UINT8(*scrambling_supported_freqs, stream);
      scrambling_supported_freqs++;
    }
  }

  buffer_allocator->free(response);
}

static void parse_read_add_on_features_supported_response(
    BT_HDR* response, bt_device_soc_add_on_features_t* supported_add_on_features,
    uint8_t *valid_bytes, uint16_t* product_id, uint16_t* response_version) {

  uint8_t* ptr = response->data + response->offset;
  uint8_t parameter_length = ptr[1];

  uint8_t* stream = read_command_complete_header(
      response, NO_OPCODE_CHECKING, 0 /* bytes after */);

  if (stream && (parameter_length > 8)) {
    STREAM_TO_UINT16(*product_id, stream);
    STREAM_TO_UINT16(*response_version, stream);

    *valid_bytes = parameter_length - 8;
    STREAM_TO_ARRAY(supported_add_on_features->as_array, stream, *valid_bytes);
  }

  buffer_allocator->free(response);
}

static void parse_read_local_simple_paring_options_response(
    BT_HDR* response, uint8_t* simple_pairing_options,
    uint8_t* maximum_encryption_key_size) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_READ_LOCAL_SIMPLE_PAIRING_OPTIONS, 2 /* bytes after */);

  CHECK(stream != NULL);
  STREAM_TO_UINT8(*simple_pairing_options, stream);
  STREAM_TO_UINT8(*maximum_encryption_key_size, stream);

  buffer_allocator->free(response);
}

static void parse_ble_read_offload_features_response(
    BT_HDR *response,
    bool *ble_offload_features_supported) {

  uint8_t *stream = read_command_complete_header(response, NO_OPCODE_CHECKING, 0 /* bytes after */);
  if(stream) {
    *ble_offload_features_supported  = true;
  } else {
    *ble_offload_features_supported  = false;
  }

  buffer_allocator->free(response);
}
static void parse_read_bd_addr_response(BT_HDR* response,
                                        RawAddress* address_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_READ_BD_ADDR, RawAddress::kLength /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_BDADDR(*address_ptr, stream);

  buffer_allocator->free(response);
}

static void parse_read_local_supported_commands_response(
    BT_HDR* response, uint8_t* supported_commands_ptr,
    size_t supported_commands_length) {
  uint8_t* stream =
      read_command_complete_header(response, HCI_READ_LOCAL_SUPPORTED_CMDS,
                                   supported_commands_length /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_ARRAY(supported_commands_ptr, stream,
                  (int)supported_commands_length);

  buffer_allocator->free(response);
}

static void parse_read_local_extended_features_response(
    BT_HDR* response, uint8_t* page_number_ptr, uint8_t* max_page_number_ptr,
    bt_device_features_t* feature_pages, size_t feature_pages_count) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_READ_LOCAL_EXT_FEATURES,
      2 + sizeof(bt_device_features_t) /* bytes after */);
  CHECK(stream != NULL);

  STREAM_TO_UINT8(*page_number_ptr, stream);
  STREAM_TO_UINT8(*max_page_number_ptr, stream);

  CHECK(*page_number_ptr < feature_pages_count);
  STREAM_TO_ARRAY(feature_pages[*page_number_ptr].as_array, stream,
                  (int)sizeof(bt_device_features_t));

  buffer_allocator->free(response);
}

static void parse_ble_read_white_list_size_response(
    BT_HDR* response, uint8_t* white_list_size_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_BLE_READ_WHITE_LIST_SIZE, 1 /* byte after */);
  CHECK(stream != NULL);
  STREAM_TO_UINT8(*white_list_size_ptr, stream);

  buffer_allocator->free(response);
}

static void parse_ble_read_supported_states_response(
    BT_HDR* response, uint8_t* supported_states, size_t supported_states_size) {
  uint8_t* stream =
      read_command_complete_header(response, HCI_BLE_READ_SUPPORTED_STATES,
                                   supported_states_size /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_ARRAY(supported_states, stream, (int)supported_states_size);

  buffer_allocator->free(response);
}

static void parse_ble_read_local_supported_features_response(
    BT_HDR* response, bt_device_features_t* supported_features) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_BLE_READ_LOCAL_SPT_FEAT,
      sizeof(bt_device_features_t) /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_ARRAY(supported_features->as_array, stream,
                  (int)sizeof(bt_device_features_t));

  buffer_allocator->free(response);
}

static void parse_ble_read_antenna_info_response(
    BT_HDR* response, bt_antenna_info_t* antenna_info) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_BLE_READ_ANTENNA_INFO,
      sizeof(bt_antenna_info_t) /* bytes after */);
  CHECK(stream != NULL);
  STREAM_TO_ARRAY(antenna_info->as_array, stream,
                  (int)sizeof(bt_antenna_info_t));

  buffer_allocator->free(response);
}

static void parse_ble_read_resolving_list_size_response(
    BT_HDR* response, uint8_t* resolving_list_size_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_BLE_READ_RESOLVING_LIST_SIZE, 1 /* bytes after */);
  if (stream) {
    STREAM_TO_UINT8(*resolving_list_size_ptr, stream);
  }
  buffer_allocator->free(response);
}

static void parse_ble_read_suggested_default_data_length_response(
    BT_HDR* response, uint16_t* ble_default_packet_length_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_BLE_READ_DEFAULT_DATA_LENGTH, 2 /* bytes after */);
  if (stream) {
    STREAM_TO_UINT16(*ble_default_packet_length_ptr, stream);
  }
  buffer_allocator->free(response);
}

static void parse_ble_read_maximum_advertising_data_length(
    BT_HDR* response, uint16_t* ble_maximum_advertising_data_length_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_LE_READ_MAXIMUM_ADVERTISING_DATA_LENGTH,
      2 /* bytes after */);
  if (stream) {
    STREAM_TO_UINT16(*ble_maximum_advertising_data_length_ptr, stream);
  }
  buffer_allocator->free(response);
}

#ifdef VLOC_FEATURE
static void parse_ble_vloc_read_local_supported_capabilities(
    BT_HDR* response, bt_device_vloc_local_features_t* local_vloc_supported_features) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_LE_VLOC_READ_LOCAL_SUPORTED_CAPABILITIES,
      VLOC_LOCAL_FEATURES_MAX_SIZE /* bytes after */);
  assert(stream != NULL);
  STREAM_TO_ARRAY(local_vloc_supported_features->as_array, stream,
                  VLOC_LOCAL_FEATURES_MAX_SIZE);
  for (int i=0; i<VLOC_LOCAL_FEATURES_MAX_SIZE; i++) {
      LOG_INFO(LOG_TAG, "%s: VLOC_FEAT[%d]: %x", __func__, i, local_vloc_supported_features->as_array[i]);
  }
  buffer_allocator->free(response);
}
#endif

static void parse_ble_set_host_feature_cmd(BT_HDR* response) {
  read_command_complete_header(
      response, HCI_BLE_SET_HOST_FEATURE, 0 /* bytes after */);
  buffer_allocator->free(response);
}

static void parse_ble_read_number_of_supported_advertising_sets(
    BT_HDR* response, uint8_t* ble_number_of_supported_advertising_sets_ptr) {
  uint8_t* stream = read_command_complete_header(
      response, HCI_LE_READ_NUMBER_OF_SUPPORTED_ADVERTISING_SETS,
      1 /* bytes after */);
  if (stream) {
    STREAM_TO_UINT8(*ble_number_of_supported_advertising_sets_ptr, stream);
  }
  buffer_allocator->free(response);
}

static void parse_set_min_encryption_key_size_response(BT_HDR* response) {
  assert(read_command_complete_header(
      response, HCI_SET_MIN_ENCRYPTION_KEY_SIZE, 0) != NULL);
}

static void parse_qll_read_local_supported_features_response(
     BT_HDR* response, bt_device_qll_local_supported_features_t* supported_features) {
  uint8_t* stream = read_command_complete_header(
                response, HCI_VS_QBCE_OCF,
       9 /* bytes after */);

   if (stream) {
     uint8_t sub_opcode;
     STREAM_TO_UINT8(sub_opcode, stream);
     if (sub_opcode == QBCE_READ_LOCAL_QLL_SUPPORTED_FEATURES)
     {
       STREAM_TO_ARRAY(supported_features->as_array, stream,
                (int)sizeof(bt_device_qll_local_supported_features_t));
     }
   } else {
     LOG_ERROR(LOG_TAG, "%s: stream null check cmnd status reason", __func__);
   }
   buffer_allocator->free(response);
}

// Internal functions

static uint8_t* read_command_complete_header(BT_HDR* response,
                                             command_opcode_t expected_opcode,
                                             size_t minimum_bytes_after) {
  uint8_t* stream = response->data + response->offset;

  // Read the event header
  uint8_t event_code;
  uint8_t parameter_length;
  STREAM_TO_UINT8(event_code, stream);
  STREAM_TO_UINT8(parameter_length, stream);

  const size_t parameter_bytes_we_read_here = 4;

  // Check the event header values against what we expect
  CHECK(event_code == HCI_COMMAND_COMPLETE_EVT);
  CHECK(parameter_length >=
        (parameter_bytes_we_read_here + minimum_bytes_after));

  // Read the command complete header
  command_opcode_t opcode;
  uint8_t status;
  STREAM_SKIP_UINT8(stream);  // skip the number of hci command packets field
  STREAM_TO_UINT16(opcode, stream);

  // Check the command complete header values against what we expect
  if (expected_opcode != NO_OPCODE_CHECKING) {
    CHECK(opcode == expected_opcode);
  }

  // Assume the next field is the status field
  STREAM_TO_UINT8(status, stream);

  if (status != HCI_SUCCESS) {
    LOG_ERROR(LOG_TAG, "%s: return status - 0x%x", __func__, status);
    return NULL;
  }

  return stream;
}

static const hci_packet_parser_t interface = {
    parse_generic_command_complete,
    parse_read_buffer_size_response,
    parse_read_local_version_info_response,
    parse_read_bd_addr_response,
    parse_read_local_supported_commands_response,
    parse_read_local_extended_features_response,
    parse_ble_read_white_list_size_response,
    parse_ble_read_buffer_size_response,
    parse_ble_read_supported_states_response,
    parse_ble_read_local_supported_features_response,
    parse_ble_read_antenna_info_response,
    parse_ble_read_resolving_list_size_response,
    parse_ble_read_suggested_default_data_length_response,
    parse_ble_read_maximum_advertising_data_length,
    parse_ble_read_number_of_supported_advertising_sets,
    parse_read_local_supported_codecs_response,
    parse_ble_read_offload_features_response,
    parse_read_scrambling_supported_freqs_response,
    parse_read_add_on_features_supported_response,
    parse_read_local_simple_paring_options_response,
    parse_ble_set_host_feature_cmd,
    parse_set_min_encryption_key_size_response,
    parse_qll_read_local_supported_features_response,
#ifdef VLOC_FEATURE
    parse_ble_vloc_read_local_supported_capabilities,
#endif
};

const hci_packet_parser_t* hci_packet_parser_get_interface() {
  buffer_allocator = buffer_allocator_get_interface();
  return &interface;
}

const hci_packet_parser_t* hci_packet_parser_get_test_interface(
    allocator_t* buffer_allocator_interface) {
  buffer_allocator = buffer_allocator_interface;
  return &interface;
}
