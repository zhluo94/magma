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

#include "broker/protos/brokerd.grpc.pb.h"
#include "GRPCReceiver.h"
#include "broker_messages_types.h"

extern "C" {
#include "intertask_interface.h"

namespace grpc {
class Status;
}  // namespace grpc
namespace magma {
namespace broker {
class BrokerAuthenticationInformationAnswer;
class BrokerPurgeUEAnswer;
class BrokerUpdateLocationAnswer;
}  // namespace broker
}  // namespace magma
}

using grpc::Status;

namespace magma {

/**
 * brokerClient is the main asynchronous client for interacting with brokerd.
 * Responses will come in a queue and call the callback passed
 * To start the client and make sure it receives calls, one must call the
 * rpc_response_loop method defined in the GRPCReceiver base class
 */
class brokerClient : public GRPCReceiver {
 public:
  // /**
  //  * Proxy a purge gRPC call to brokerd
  //  */
  static void purge_ue(
    const char *imsi,
    std::function<void(Status, broker::BrokerPurgeUEAnswer)> callback);

  /**
   * Proxy a purge gRPC call to brokerd
   */
  static void authentication_info_req(
    const broker_auth_info_req_t *const msg,
    std::function<void(Status, broker::BrokerAuthenticationInformationAnswer)> callbk);

  // /**
  //  * Proxy a purge gRPC call to brokerd
  //  */
  static void update_location_request(
    const broker_update_location_req_t *const msg,
    std::function<void(Status, broker::BrokerUpdateLocationAnswer)> callback);

 public:
  brokerClient(brokerClient const &) = delete;
  void operator=(brokerClient const &) = delete;

 private:
  brokerClient();
  static brokerClient &get_instance();
  std::unique_ptr<broker::Brokerd::Stub> stub_;
  static const uint32_t RESPONSE_TIMEOUT = 3; // seconds
};

// There are 3 services which can handle authentication:
// 1) Local subscriberdb
// 2) Subscriberdb in the cloud (EPS Authentication)
// 3) S6a Proxy running in the FeG
// When relay_enabled is true, then auth requests are sent to the S6a Proxy.
// Otherwise, if cloud_subscriberdb_enabled is true, then auth requests are
// sent to the EPS Authentication service.
// If neither flag is true, then a local instance of subscriberdb receives the
// auth messages.
// bool get_s6a_relay_enabled(void);
// bool get_cloud_subscriberdb_enabled(void);

} // namespace magma
