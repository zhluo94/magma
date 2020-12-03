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
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <thread> // std::thread
#include <iostream>
#include <utility>

#include "lte/protos/mconfig/mconfigs.pb.h"
#include "MConfigLoader.h"
#include "brokerClient.h"
#include "ServiceRegistrySingleton.h"
#include "itti_msg_to_proto_msg.h"
#include "broker/protos/brokerd.pb.h"

namespace grpc {
class Status;
}  // namespace grpc

using grpc::Status;
#define MME_SERVICE "mme"

namespace magma {
using namespace broker;

// static bool read_mme_relay_enabled(void);

// static bool read_mme_cloud_subscriberdb_enabled(void);

// static const bool relay_enabled = read_mme_relay_enabled();

// static const bool cloud_subscriberdb_enabled = read_mme_cloud_subscriberdb_enabled();

// Relay enabled must be controlled by mconfigs in ONE place
// and mme app should be restarted on the config changes.
//
// The relay decision is encapsulated in Broker Client API, it is STATIC
// and CONSTANT during the application's lifetime and can only be queried
// (not changed)

// bool get_s6a_relay_enabled(void)
// {
//   return relay_enabled;
// }

// bool get_cloud_subscriberdb_enabled(void)
// {
//   return cloud_subscriberdb_enabled;
// }

// static bool read_mme_relay_enabled(void)
// {
//   magma::mconfig::MME mconfig;
//   magma::MConfigLoader loader;

//   if (!loader.load_service_mconfig(MME_SERVICE, &mconfig)) {
//     std::cout << "[INFO] Unable to load mconfig for mme, S6a relay is disabled" << std::endl;
//     return false; // default is - relay disabled
//   }
//   return mconfig.relay_enabled();
// }

// static bool read_mme_cloud_subscriberdb_enabled(void)
// {
//   magma::mconfig::MME mconfig;
//   magma::MConfigLoader loader;

//   if (!loader.load_service_mconfig(MME_SERVICE, &mconfig)) {
//     std::cout << "[INFO] Unable to load mconfig for mme, cloud subscriberdb is disabled" << std::endl;
//     return false; // default is - cloud subscriberdb disabled
//   }
//   return mconfig.cloud_subscriberdb_enabled();
// }

brokerClient &brokerClient::get_instance()
{
  static brokerClient client_instance;
  return client_instance;
}

brokerClient::brokerClient()
{
   // Create channel based on relay_enabled and cloud_subscriberdb_enabled
   // flags. If relay_enabled is true, then create a channel towards the FeG.
   // Otherwise, create a channel towards either local or cloud-based
   // subscriberdb.
  // if (get_s6a_relay_enabled() == true) {
  //   auto channel = ServiceRegistrySingleton::Instance()->GetGrpcChannel(
  //     "s6a_proxy", ServiceRegistrySingleton::CLOUD);
  //   // Create stub for S6aProxy gRPC service
  //   stub_ = S6aProxy::NewStub(channel);
  // }
  // else {
  //   auto channel = ServiceRegistrySingleton::Instance()->GetGrpcChannel(
  //     "subscriberdb", ServiceRegistrySingleton::LOCAL);
  //   // Create stub for subscriberdb gRPC service
  //   stub_ = S6aProxy::NewStub(channel);
  // }

  auto channel = ServiceRegistrySingleton::Instance()->GetGrpcChannel("brokerd", ServiceRegistrySingleton::CLOUD);
  stub_ = Brokerd::NewStub(channel);

  std::thread resp_loop_thread([&]() { rpc_response_loop(); });
  resp_loop_thread.detach();
}


void brokerClient::authentication_info_req(
  const broker_auth_info_req_t *const msg,
  std::function<void(Status, broker::BrokerAuthenticationInformationAnswer)> callbk)
{
  brokerClient &client = get_instance();
  BrokerAuthenticationInformationRequest proto_msg =
    convert_itti_broker_authentication_info_req_to_proto_msg(msg);
  // Create a raw response pointer that stores a callback to be called when the
  // gRPC call is answered
  auto resp = new AsyncLocalResponse<BrokerAuthenticationInformationAnswer>(
    std::move(callbk), RESPONSE_TIMEOUT);

  // Create a response reader for the `authentication_info_req` RPC call.
  // This reader stores the client context, the request to pass in, and
  // the queue to add the response to when done

  auto resp_rdr = client.stub_->AsyncBrokerAuthenticationInformation(
    resp->get_context(), proto_msg, &client.queue_);

  // Set the reader for the response. This executes the `Auth_Info_Req`
  // response using the response reader. When it is done, the callback stored
  // in `resp` will be called
  resp->set_response_reader(std::move(resp_rdr));
}

void brokerClient::update_location_request(
  const broker_update_location_req_t *const msg,
  std::function<void(Status, broker::BrokerUpdateLocationAnswer)> callbk)
{
  brokerClient &client = get_instance();
  BrokerUpdateLocationRequest proto_msg =
    convert_itti_broker_update_location_request_to_proto_msg(msg);
  // Create a raw response pointer that stores a callback to be called when the
  // gRPC call is answered
  auto resp = new AsyncLocalResponse<BrokerUpdateLocationAnswer>(
    std::move(callbk), RESPONSE_TIMEOUT);

  // Create a response reader for the `update_location_request` RPC call.
  // This reader stores the client context, the request to pass in, and
  // the queue to add the response to when done

  auto resp_rdr = client.stub_->AsyncBrokerUpdateLocation(
    resp->get_context(), proto_msg, &client.queue_);

  // Set the reader for the response. This executes the `Location Update Req`
  // response using the response reader. When it is done, the callback stored
  // in `resp` will be called
  resp->set_response_reader(std::move(resp_rdr));
}

void brokerClient::purge_ue(
  const char *imsi,
  std::function<void(Status, BrokerPurgeUEAnswer)> callbk)
{
  brokerClient &client = get_instance();

  // Create a raw response pointer that stores a callback to be called when the
  // gRPC call is answered
  auto resp =
    new AsyncLocalResponse<BrokerPurgeUEAnswer>(std::move(callbk), RESPONSE_TIMEOUT);

  // Create a response reader for the `PurgeUE` RPC call. This reader
  // stores the client context, the request to pass in, and the queue to add
  // the response to when done
  BrokerPurgeUERequest puRequest;
  puRequest.set_user_name(imsi);
  auto resp_rdr =
    client.stub_->AsyncBrokerPurgeUE(resp->get_context(), puRequest, &client.queue_);

  // Set the reader for the response. This executes the `PurgeUE`
  // response using the response reader. When it is done, the callback stored
  // in `resp` will be called
  resp->set_response_reader(std::move(resp_rdr));
}

} // namespace magma
