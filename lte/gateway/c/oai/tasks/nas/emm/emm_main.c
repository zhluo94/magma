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

#include <string.h>

#include "log.h"
#include "common_defs.h"
#include "emm_main.h"
#include "mme_config.h"
#include "mme_api.h"

// added for brokerd utelco
static void _read_ut_keys(void);

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
void emm_main_initialize(const mme_config_t *mme_config_p)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  /*
   * Retreive MME supported configuration data
   */
  memset(&_emm_data.conf, 0, sizeof(_emm_data.conf));
  if (mme_api_get_emm_config(&_emm_data.conf, mme_config_p) != RETURNok) {
    OAILOG_ERROR(
      LOG_NAS_EMM, "EMM-MAIN  - Failed to get MME configuration data");
  }
  // Added brokerd utelco
  _read_ut_keys();

  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

// added for brokerd utelco
static void _read_ut_keys(void)
{
  OAILOG_INFO(LOG_NAS_EMM, "EMM-MAIN  - read keys for utelco");
  FILE * pub_br_ec_fp  = fopen("/home/vagrant/key_files/br_ec_pub.pem", "rb");
  FILE * pri_ut_ec_fp  = fopen("/home/vagrant/key_files/ut_ec_pri.pem", "rb");
  FILE * pri_ut_rsa_fp = fopen("/home/vagrant/key_files/ut_rsa_pri.pem", "rb");
  if(pub_br_ec_fp == NULL || pri_ut_ec_fp == NULL || pri_ut_rsa_fp == NULL) {
    OAILOG_ERROR(LOG_NAS_EMM, "EMM-MAIN  - can't open key files");
    return;
  }
  EVP_PKEY *pkey_br_ec_pub, *pkey_ut_ec_pri, *pkey_ut_rsa_pri;
  pkey_br_ec_pub = PEM_read_PUBKEY(pub_br_ec_fp, NULL, 0, NULL);
  pkey_ut_ec_pri = PEM_read_PrivateKey(pri_ut_ec_fp, NULL, 0, NULL); 
  pkey_ut_rsa_pri = PEM_read_PrivateKey(pri_ut_rsa_fp, NULL, 0, NULL);   
  fclose(pub_br_ec_fp);
  fclose(pri_ut_ec_fp);
  fclose(pri_ut_rsa_fp);
  _ut_keys.br_public_ecdsa = EVP_PKEY_get1_EC_KEY(pkey_br_ec_pub);
  _ut_keys.ut_private_ecdsa = EVP_PKEY_get1_EC_KEY(pkey_ut_ec_pri);
  _ut_keys.ut_private_rsa = EVP_PKEY_get1_RSA(pkey_ut_rsa_pri);
  OAILOG_INFO(LOG_NAS_EMM, "EMM-MAIN  - extract EC and RSA keys for utelco");
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Mobility Management clean up procedure   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void emm_main_cleanup(void)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
