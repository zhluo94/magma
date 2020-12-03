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

//#include "MConfigLoader.h"
#include "SubscriberClient.h"
#include "ServiceRegistrySingleton.h"
#include "lte/protos/subscriberdb.pb.h"

namespace grpc {
class Status;
}  // namespace grpc

using grpc::Status;

namespace magma {
using namespace lte;
using namespace orc8r;

SubscriberClient &SubscriberClient::get_instance()
{
  static SubscriberClient client_instance;
  return client_instance;
}

SubscriberClient::SubscriberClient()
{
  auto channel = ServiceRegistrySingleton::Instance()->GetGrpcChannel(
      "subscriberdb", ServiceRegistrySingleton::LOCAL);
  stub_ = SubscriberDB::NewStub(channel);

  std::thread resp_loop_thread([&]() { rpc_response_loop(); });
  resp_loop_thread.detach();
}

void SubscriberClient::add_sub(
  const char *sub_data, 
  int sub_len,
  std::function<void(Status, orc8r::Void)> callbk)
{
  SubscriberClient &client = get_instance();

  auto resp = new AsyncLocalResponse<orc8r::Void>(std::move(callbk), RESPONSE_TIMEOUT);

  std::string sub_data_str(sub_data, sub_len);
  lte::SubscriberData sub_data_proto;
  if(!sub_data_proto.ParseFromString(sub_data_str)) {
    std::cout << "[ERROR] Fails to parse from string of length " << sub_data_str.length() << std::endl;
    return;
  }
  std::cout << "[INFO] Add Subscriber RPC call for IMISI: " << sub_data_proto.sid().id() << std::endl;
  auto resp_rdr = client.stub_->AsyncAddSubscriber(resp->get_context(), sub_data_proto, &client.queue_);

  resp->set_response_reader(std::move(resp_rdr));
}

void SubscriberClient::del_sub(
  const char *imsi,
  std::function<void(Status, orc8r::Void)> callbk)
{
  SubscriberClient &client = get_instance();

  auto resp = new AsyncLocalResponse<orc8r::Void>(std::move(callbk), RESPONSE_TIMEOUT);

  lte::SubscriberID sub_id_proto;
  sub_id_proto.set_id(imsi);

  auto resp_rdr = client.stub_->AsyncDeleteSubscriber(resp->get_context(), sub_id_proto, &client.queue_);

  resp->set_response_reader(std::move(resp_rdr));
}

} // namespace magma
