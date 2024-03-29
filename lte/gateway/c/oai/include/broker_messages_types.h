/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
/*! \fil  broker_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_BROKER_MESSAGES_TYPES_SEEN
#define FILE_BROKER_MESSAGES_TYPES_SEEN

#include <stdint.h>

#include "3gpp_23.003.h"
#include "common_types.h"
#include "security_types.h"
#include "s6a_messages_types.h"

#define BROKER_AUTH_INFO_REQ(mSGpTR) (mSGpTR)->ittiMsg.broker_auth_info_req
#define BROKER_AUTH_INFO_ANS(mSGpTR) (mSGpTR)->ittiMsg.broker_auth_info_ans

#define BROKER_UPDATE_LOCATION_REQ(mSGpTR) (mSGpTR)->ittiMsg.broker_update_location_req
#define BROKER_UPDATE_LOCATION_ANS(mSGpTR) (mSGpTR)->ittiMsg.broker_update_location_ans
#define BROKER_PURGE_UE_REQ(mSGpTR) (mSGpTR)->ittiMsg.broker_purge_ue_req
#define BROKER_PURGE_UE_ANS(mSGpTR) (mSGpTR)->ittiMsg.broker_purge_ue_ans
#define AUTS_LENGTH 14
#define RESYNC_PARAM_LENGTH AUTS_LENGTH + RAND_LENGTH_OCTETS
// for brokerd uTelco
#define TOKEN_LENGTH 128
#define UE_SIGNATURE_LENGTH 50
#define UT_SIGNATURE_LENGTH 50

//typedef s6a_auth_info_req_t broker_auth_info_req_t;
// typedef s6a_auth_info_ans_t broker_auth_info_ans_t;
typedef s6a_update_location_req_t broker_update_location_req_t;
typedef s6a_update_location_ans_t broker_update_location_ans_t;
typedef s6a_purge_ue_req_t broker_purge_ue_req_t;
typedef s6a_purge_ue_ans_t broker_purge_ue_ans_t;

typedef struct broker_auth_info_req_s {
  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;
  plmn_t visited_plmn;
  /* Number of vectors to retrieve from HSS, should be equal to one */
  uint8_t nb_of_vectors;

  /* Bit to indicate that USIM has requested a re-synchronization of SQN */
  unsigned re_synchronization : 1;
  /* AUTS to provide to AUC.
   * Only present and interpreted if re_synchronization == 1.
   */
  uint8_t resync_param[RAND_LENGTH_OCTETS + AUTS_LENGTH]; // TODO: need to change this
  uint8_t ue_br_token[TOKEN_LENGTH];
  uint8_t ue_br_token_ue_sig[UE_SIGNATURE_LENGTH];
  uint8_t ue_br_token_ut_sig[UT_SIGNATURE_LENGTH];
} broker_auth_info_req_t;

typedef struct broker_auth_info_ans_s {
  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;

  /* Result of the authentication information procedure */
  s6a_result_t result;
  /* Authentication info containing the vector(s) */
  broker_authentication_info_t auth_info;
} broker_auth_info_ans_t;

// typedef struct broker_purge_ue_req_s {
//   char imsi[IMSI_BCD_DIGITS_MAX + 1];
//   uint8_t imsi_length;
// } broker_purge_ue_req_t;

// typedef struct broker_purge_ue_ans_s {
//   char imsi[IMSI_BCD_DIGITS_MAX + 1];
//   uint8_t imsi_length;
//   /* Result of the purge ue procedure */
//   s6a_result_t result;
//   unsigned freeze_m_tmsi : 1;
//   unsigned freeze_p_tmsi : 1;
// } broker_purge_ue_ans_t;

// typedef struct broker_update_location_req_s {
// #define SKIP_SUBSCRIBER_DATA (0x1)
//   unsigned skip_subscriber_data : 1;
// #define INITIAL_ATTACH (0x1)
//   unsigned initial_attach : 1;

//   char imsi[IMSI_BCD_DIGITS_MAX + 1]; // username
//   uint8_t imsi_length;                // username

//   plmn_t visited_plmn; // visited plmn id
//   rat_type_t rat_type; // rat type

//   // missing                           // origin host
//   // missing                           // origin realm

//   // missing                           // destination host
//   // missing                           // destination realm
//   uint8_t presencemask;
// #define BROKER_PDN_CONFIG_VOICE_DOM_PREF (1 << 0)
// #define HOMOGENEOUS_IMS_VOICE_OVER_PS_SUPPORTED (0x1)
//   voice_domain_preference_and_ue_usage_setting_t
//     voice_dom_pref_ue_usg_setting;
// } broker_update_location_req_t;

// typedef struct broker_update_location_ans_s {
//   s6a_result_t result; // Result of the update location request procedure
//   subscription_data_t subscription_data; // subscriber status,
//   // Maximum Requested Bandwidth Uplink, downlink
//   // access restriction data
//   // msisdn
//   //apn_config_profile_t  apn_config_profile;// APN configuration profile

//   network_access_mode_t access_mode;
//   rau_tau_timer_t rau_tau_timer;
//   char imsi[IMSI_BCD_DIGITS_MAX + 1];
//   uint8_t imsi_length;

// } broker_update_location_ans_t;
#endif /* FILE_BROKER_MESSAGES_TYPES_SEEN */
