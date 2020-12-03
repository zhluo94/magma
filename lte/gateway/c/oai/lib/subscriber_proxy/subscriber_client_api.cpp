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

#include <grpcpp/impl/codegen/status.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <iostream>
#include <conversions.h>

#include "subscriber_client_api.h"
#include "SubscriberClient.h"
#include "common_types.h"
#include "lte/protos/subscriberdb.pb.h"

extern "C" {
}

using namespace magma;
using namespace magma::orc8r;

bool subscriber_add_sub(const char *sub_data, int sub_len)
{
  if (sub_data == nullptr) {
    return false;
  }
  magma::SubscriberClient::add_sub(
    sub_data, sub_len, [](grpc::Status status, orc8r::Void response) {
      auto log_level = "ERROR";
      if (status.ok()) {
        log_level = "INFO";
      }
      // For now, do nothing, just log
      std::cout << "[" << log_level << "] AddSubscriber Response"
                      << "; Status: " << status.error_message() << std::endl;
      return;
    });
}

bool subscriber_del_sub(const char *imsi)
{
  if (imsi == nullptr) {
    return false;
  }
  magma::SubscriberClient::del_sub(
    imsi,
    [imsiStr = std::string(imsi)](
      grpc::Status status, orc8r::Void response) {
      auto log_level = "ERROR";
      if (status.ok()) {
        log_level = "INFO";
      }
      // For now, do nothing, just log
      std::cout << "[" << log_level << "] DeleteSubscriber Response for IMSI: " << imsiStr
                      << "; Status: " << status.error_message() << std::endl;
      return;
    });
}
