/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-----------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#pragma once

#include <gmp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "intertask_interface.h"
#include "broker_messages_types.h"

/**
 * broker_authentication_info_req is an asynchronous call that forwards broker AIR to
 * brokerd
 */
bool broker_authentication_info_req(const broker_auth_info_req_t *air_p);

/**
 * broker_purge is an asynchronous call that forwards Broker PU to Federation Gateway
 * if S6a Relay is enabled by mconfig
 */
bool broker_purge_ue(const char *imsi);

/**
 * broker_update_location_req is an asynchronous call that forwards S6a ULR to
 * Federation Gateway, if S6a Relay is enabled by mconfig
 */
bool broker_update_location_req(const broker_update_location_req_t *const ulr_p);

#ifdef __cplusplus
}
#endif
