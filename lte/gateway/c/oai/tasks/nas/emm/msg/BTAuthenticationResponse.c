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

#include <stdint.h>
#include <stdbool.h>

#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "BTAuthenticationResponse.h"

int decode_bt_authentication_response(
  bt_authentication_response_msg *authentication_response,
  uint8_t *buffer,
  uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(
    buffer, BT_AUTHENTICATION_RESPONSE_MINIMUM_LENGTH, len);

  /*
   * Decoding mandatory fields
   */
  if (
    (decoded_result = decode_bt_authentication_response_parameter_ie(
       &authentication_response->btauthenticationresponseparameter,
       true,
       buffer + decoded,
       len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  return decoded;
}

int encode_bt_authentication_response(
  bt_authentication_response_msg *authentication_response,
  uint8_t *buffer,
  uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(
    buffer, BT_AUTHENTICATION_RESPONSE_MINIMUM_LENGTH, len);

  if (
    (encode_result = encode_bt_authentication_response_parameter_ie(
       authentication_response->btauthenticationresponseparameter,
       true,
       buffer + encoded,
       len - encoded)) < 0) //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}
