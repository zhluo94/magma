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

#ifndef FILE_USAGE_REPORT_REQUEST_SEEN
#define FILE_USAGE_REPORT_REQUEST_SEEN

#include <stdint.h>

#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "NasKeySetIdentifier.h"
#include "3gpp_23.003.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"

/* Minimum length macro. Formed by minimum length of each mandatory field */
#define USAGE_REPORT_REQUEST_MINIMUM_LENGTH                                  \
  (USAGE_REPORT_PARAMETER_REPORT_ID_IE_MIN_LENGTH - 2)

/* Maximum length macro. Formed by maximum length of each field */
#define USAGE_REPORT_REQUEST_MAXIMUM_LENGTH                                  \
  (USAGE_REPORT_PARAMETER_REPORT_ID_IE_MAX_LENGTH)

/*
 * Message name: Usage Report request
 * Description: This message is sent by the network to the UE to fetch a UE-generated report.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct usage_report_request_msg_tag {
  /* Mandatory fields */
  eps_protocol_discriminator_t protocoldiscriminator : 4;
  security_header_type_t securityheadertype : 4;
  message_type_t messagetype;
  usage_report_parameter_report_id_t report_id;
} usage_report_request_msg;

int decode_usage_report_request(
  usage_report_request_msg *usagereportrequest,
  uint8_t *buffer,
  uint32_t len);

int encode_usage_report_request(
  usage_report_request_msg *usagereportrequest,
  uint8_t *buffer,
  uint32_t len);

#endif /* ! defined(FILE_USAGE_REPORT_REQUEST_SEEN) */
