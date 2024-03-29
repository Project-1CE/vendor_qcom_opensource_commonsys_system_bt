/******************************************************************************
 *
 *  Copyright (C) 2016 The Android Open Source Project
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 ******************************************************************************/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// Interface to the A2DP SBC Encoder
//

#ifndef A2DP_SBC_ENCODER_H
#define A2DP_SBC_ENCODER_H

#include "a2dp_codec_api.h"
#include "osi/include/time.h"

// Loads the A2DP SBC encoder.
// Return true on success, otherwise false.
bool A2DP_LoadEncoderSbc(void);

// Unloads the A2DP SBC encoder.
void A2DP_UnloadEncoderSbc(void);

// Initialize the A2DP SBC encoder.
// |p_peer_params| contains the A2DP peer information
// The current A2DP codec config is in |a2dp_codec_config|.
// |read_callback| is the callback for reading the input audio data.
// |enqueue_callback| is the callback for enqueueing the encoded audio data.
void a2dp_sbc_encoder_init(const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
                           A2dpCodecConfig* a2dp_codec_config,
                           a2dp_source_read_callback_t read_callback,
                           a2dp_source_enqueue_callback_t enqueue_callback);

// Cleanup the A2DP SBC encoder.
void a2dp_sbc_encoder_cleanup(void);

// Reset the feeding for the A2DP SBC encoder.
void a2dp_sbc_feeding_reset(void);

// Flush the feeding for the A2DP SBC encoder.
void a2dp_sbc_feeding_flush(void);

// Get the A2DP SBC encoder interval (in milliseconds).
period_ms_t a2dp_sbc_get_encoder_interval_ms(void);

// Prepare and send A2DP SBC encoded frames.
// |timestamp_us| is the current timestamp (in microseconds).
void a2dp_sbc_send_frames(uint64_t timestamp_us);

// Calculsate sbc bitrate for offload mode
// |a2dp_codec_config| is codec config
// |peer_edr| flag for peer supports edr
// Returns |uint16_t| calculated bitrare
uint16_t a2dp_sbc_calulate_offload_bitrate(A2dpCodecConfig* a2dp_codec_config,
                                           bool peer_edr, const uint8_t* src_codec_info);
#endif  // A2DP_SBC_ENCODER_H
