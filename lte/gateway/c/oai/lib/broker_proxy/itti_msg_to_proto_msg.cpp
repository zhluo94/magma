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


#include "itti_msg_to_proto_msg.h"
#include "security_types.h"

extern "C" {
#include "ie_to_bytes.h"
}

namespace magma {

BrokerAuthenticationInformationRequest
convert_itti_broker_authentication_info_req_to_proto_msg(
  const broker_auth_info_req_t *const msg)
{
  BrokerAuthenticationInformationRequest ret;
  ret.Clear();

  /*
   * Adding the User-Name (IMSI)
   */
  ret.set_user_name(msg->imsi, msg->imsi_length);

  /*
   * Adding the visited plmn id
   */
  char plmn[IE_LENGTH_PLMN];
  plmn_to_bytes(&msg->visited_plmn, plmn);
  ret.set_visited_plmn(plmn, IE_LENGTH_PLMN);

  /*
   * Add the number of requested vectors
   */
  ret.set_num_requested_eutran_vectors(msg->nb_of_vectors);

  /*
   * We want to use the vectors immediately in HSS so we have to add
   * * * * the Immediate-Response-Preferred AVP.
   * * * * Value of this AVP is not significant.
   */
  ret.set_immediate_response_preferred(0);

  /*
   * Re-synchronization information containing the AUTS computed at USIM
   */

  if (msg->re_synchronization){
    ret.set_resync_info(msg->resync_param, (RAND_LENGTH_OCTETS + AUTS_LENGTH));
  }

  // Added for brokerd-uTelco
  ret.set_ue_br_token(msg->ue_br_token, TOKEN_LENGTH);
  ret.set_ue_br_token_ue_sig(msg->ue_br_token_ue_sig, UE_SIGNATURE_LENGTH);
  ret.set_ue_br_token_ut_sig(msg->ue_br_token_ut_sig, UT_SIGNATURE_LENGTH);

  return ret;
}

BrokerUpdateLocationRequest convert_itti_broker_update_location_request_to_proto_msg(
  const broker_update_location_req_t *const msg)
{
  BrokerUpdateLocationRequest ret;
  ret.Clear();

  /*
   * Adding the User-Name (IMSI)
   */
  ret.set_user_name(msg->imsi, msg->imsi_length);

  /*
   * Adding the visited plmn id
   */
  char plmn[IE_LENGTH_PLMN];
  plmn_to_bytes(&msg->visited_plmn, plmn);
  ret.set_visited_plmn(plmn, IE_LENGTH_PLMN);

  /*
   * Set the ulr-flags as indicated by upper layer
   * Set the skip_subscriber_data as indicated by upper layer
   */
  if (msg->skip_subscriber_data) {
    ret.set_skip_subscriber_data(SKIP_SUBSCRIBER_DATA);
  }

  /*
   * Set the initial_attach
   */
  if (msg->initial_attach) {
    ret.set_initial_attach(INITIAL_ATTACH);
  }
  return ret;
}
} // namespace magma
