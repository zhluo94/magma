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
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */


#pragma once

#include <gmp.h>

#include "broker/protos/brokerd.pb.h"
#include "broker/protos/brokerd.grpc.pb.h"
#include "broker_messages_types.h"

extern "C" {
#include "intertask_interface.h"

namespace magma {
namespace broker {
class BrokerAuthenticationInformationAnswer;
class BrokerUpdateLocationAnswer;
}  // namespace feg
}  // namespace magma
}

namespace magma {
using namespace broker;

void convert_proto_msg_to_itti_broker_auth_info_ans(
  BrokerAuthenticationInformationAnswer msg,
  broker_auth_info_ans_t *itti_msg);

void convert_proto_msg_to_itti_broker_update_location_ans(
  BrokerUpdateLocationAnswer msg,
  broker_update_location_ans_t *itti_msg);
} // namespace magma
