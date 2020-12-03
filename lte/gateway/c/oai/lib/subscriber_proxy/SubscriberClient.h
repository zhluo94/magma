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
#include <grpc++/grpc++.h>
#include <stdint.h>
#include <functional>
#include <memory>

#include "lte/protos/subscriberdb.grpc.pb.h"
#include "GRPCReceiver.h"

extern "C" {
#include "intertask_interface.h"

namespace grpc {
class Status;
}  // namespace grpc
namespace magma {
namespace orc8r {
class Void;
}  // namespace orc8r
}  // namespace magma
}

using grpc::Status;

namespace magma {

/**
 * SubscriberClient is the main asynchronous client for interacting with Subscriberdb.
 * Responses will come in a queue and call the callback passed
 * To start the client and make sure it receives calls, one must call the
 * rpc_response_loop method defined in the GRPCReceiver base class
 */
class SubscriberClient : public GRPCReceiver {
 public:
  /**
   * Proxy a add subscriber gRPC call to Subscriberdb
   */
  static void add_sub(
    const char *sub_data, int sub_len,
    std::function<void(Status, orc8r::Void)> callback);

  /**
   * Proxy a delete subscriber gRPC call to Subscriberdb
   */
  static void del_sub(
    const char *imsi,
    std::function<void(Status, orc8r::Void)> callbk);

 public:
  SubscriberClient(SubscriberClient const &) = delete;
  void operator=(SubscriberClient const &) = delete;

 private:
  SubscriberClient();
  static SubscriberClient &get_instance();
  std::unique_ptr<lte::SubscriberDB::Stub> stub_;
  static const uint32_t RESPONSE_TIMEOUT = 3; // seconds
};

} // namespace magma
