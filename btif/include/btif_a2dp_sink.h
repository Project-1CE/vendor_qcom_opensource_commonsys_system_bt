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

#ifndef BTIF_A2DP_SINK_H
#define BTIF_A2DP_SINK_H

#include <inttypes.h>
#include <stdbool.h>

#include "bt_types.h"
#include "bta_av_api.h"

//
// Audio focus state for audio track.
//
// NOTE: The values must be same as:
//  - A2dpSinkStreamingStateMachine.STATE_FOCUS_LOST = 0
//  - A2dpSinkStreamingStateMachine.STATE_FOCUS_GRANTED = 1
//
typedef enum {
  BTIF_A2DP_SINK_FOCUS_NOT_GRANTED = 0,
  BTIF_A2DP_SINK_FOCUS_GRANTED = 1
} btif_a2dp_sink_focus_state_t;

// Initialize and startup the A2DP Sink module.
// This function should be called by the BTIF state machine prior to using the
// module.
bool btif_a2dp_sink_startup(void);

// Shutdown and cleanup the A2DP Sink module.
// This function should be called by the BTIF state machine during
// graceful shutdown and cleanup.
void btif_a2dp_sink_shutdown(void);

// Get the audio sample rate for the A2DP Sink module.
tA2DP_SAMPLE_RATE btif_a2dp_sink_get_sample_rate(void);

// Get Audio Track Latency
uint32_t get_audiotrack_latency();

// Get the audio channel count for the A2DP Sink module.
tA2DP_CHANNEL_COUNT btif_a2dp_sink_get_channel_count(void);

// Get the audio bits per sample for the A2DP Sink module.
tA2DP_BITS_PER_SAMPLE btif_a2dp_sink_get_bits_per_sample(void);

// Update the decoder for the A2DP Sink module.
// |p_codec_info| contains the new codec information.
void btif_a2dp_sink_update_decoder(const uint8_t* p_codec_info);

// Process 'idle' request from the BTIF state machine during initialization.
void btif_a2dp_sink_on_idle(void);

// Process 'stop' request from the BTIF state machine to stop A2DP streaming.
// |p_av_suspend| is the data associated with the request - see
// |tBTA_AV_SUSPEND|.
void btif_a2dp_sink_on_stopped(tBTA_AV_SUSPEND* p_av_suspend);

// Process 'suspend' request from the BTIF state machine to suspend A2DP
// streaming.
// |p_av_suspend| is the data associated with the request - see
// |tBTA_AV_SUSPEND|.
void btif_a2dp_sink_on_suspended(tBTA_AV_SUSPEND* p_av_suspend);

// Enable/disable discarding of received A2DP frames.
// If |enable| is true, the discarding is enabled, otherwise is disabled.
void btif_a2dp_sink_set_rx_flush(bool enable);

// Enqueue a buffer to the A2DP Sink queue. If the queue has reached its
// maximum size |MAX_INPUT_A2DP_FRAME_QUEUE_SZ|, the oldest buffer is
// removed from the queue.
// |p_buf| is the buffer to enqueue.
// Returns the number of buffers in the Sink queue after the enqueing.
uint8_t btif_a2dp_sink_enqueue_buf(BT_HDR* p_buf);

// Dump debug-related information for the A2DP Sink module.
// |fd| is the file descriptor to use for writing the ASCII formatted
// information.
void btif_a2dp_sink_debug_dump(int fd);

// Update the A2DP Sink related metrics.
// This function should be called before collecting the metrics.
void btif_a2dp_sink_update_metrics(void);

// Create a request to set the audio focus state for the audio track.
// |state| is the new state value - see |btif_a2dp_sink_focus_state_t|
// for valid values.
void btif_a2dp_sink_set_focus_state_req(btif_a2dp_sink_focus_state_t state);

// Set the audio track gain for the audio track.
// |gain| is the audio track gain value to use.
void btif_a2dp_sink_set_audio_track_gain(float gain);

void btif_a2dp_sink_on_started(tBTA_AV_START* p_av_start);
#endif /* BTIF_A2DP_SINK_H */
