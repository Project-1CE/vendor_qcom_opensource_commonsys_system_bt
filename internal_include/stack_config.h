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
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************/
/*******************************************************************************
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 *******************************************************************************/

#pragma once

#include <stdbool.h>

#include "module.h"
#include "osi/include/config.h"

static const char STACK_CONFIG_MODULE[] = "stack_config_module";

typedef struct {
  bool (*get_trace_config_enabled)(void);
  bool (*get_pts_secure_only_mode)(void);
  bool (*get_pts_conn_updates_disabled)(void);
  bool (*get_pts_crosskey_sdp_disable)(void);
  const std::string* (*get_pts_smp_options)(void);
  int (*get_pts_smp_failure_case)(void);
  bool (*get_pts_le_nonconn_adv_enabled)(void);
  bool (*get_pts_le_conn_nondisc_adv_enabled)(void);
  bool (*get_pts_le_sec_request_disabled)(void);
  bool (*get_pts_le_fresh_pairing_enabled)(void);
  int (*get_pts_l2cap_le_insuff_enc_result)(void);
  int (*get_pts_smp_generate_invalid_public_key)(void);
  int (*get_pts_bredr_auth_req)(void);
  bool (*get_pts_bredr_secureconnection_host_support_disabled)(void);
  int (*get_pts_bredr_invalid_encryption_keysize)(void);
  int (*get_pts_le_enc_disable)(void);
  int (*get_pts_smp_disable_h7_support)(void);
  bool (*get_pts_service_chg_indication_disable)(void);
  bool (*get_pts_configure_svc_chg_indication)(void);
  bool (*get_pts_save_db_hash)(void);
  bool (*get_pts_enable_authorization_encr_data_key)(void);
  config_t* (*get_all)(void);
} stack_config_t;

const stack_config_t* stack_config_get_interface(void);
