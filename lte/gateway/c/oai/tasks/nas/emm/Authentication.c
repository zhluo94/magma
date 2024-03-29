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
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "log.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "common_types.h"
#include "conversions.h"
#include "3gpp_requirements_24.301.h"
#include "3gpp_24.008.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "nas_timer.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "emm_cause.h"
#include "service303.h"
#include "EmmCommon.h"
#include "3gpp_23.003.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "3gpp_36.401.h"
#include "AuthenticationResponse.h"
#include "TrackingAreaIdentity.h"
#include "common_defs.h"
#include "emm_asDef.h"
#include "emm_cnDef.h"
#include "emm_fsm.h"
#include "emm_regDef.h"
#include "mme_app_state.h"
#include "nas_procedures.h"
#include "s6a_messages_types.h"
#include "nas/securityDef.h"
#include "security_types.h"
#include "intertask_interface.h"
#include "nas_proc.h"
// added for brokerd utelco
#include "subscriber_client_api.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
    Internal data handled by the authentication procedure in the UE
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
    Internal data handled by the authentication procedure in the MME
   --------------------------------------------------------------------------
*/
// callbacks for authentication procedure
static void _authentication_t3460_handler(void *args);
static int _authentication_ll_failure(
  struct emm_context_s *emm_context,
  struct nas_emm_proc_s *emm_proc);
static int _authentication_non_delivered_ho(
  struct emm_context_s *emm_context,
  struct nas_emm_proc_s *emm_proc);
static int _authentication_abort(
  struct emm_context_s *emm_context,
  struct nas_base_proc_s *base_proc);

static int _start_authentication_information_procedure(
  struct emm_context_s *emm_context,
  nas_emm_auth_proc_t *const auth_proc,
  const_bstring auts);
static int _auth_info_proc_success_cb(struct emm_context_s *emm_ctx);
static int _auth_info_proc_failure_cb(struct emm_context_s *emm_ctx);
// added for brokerd utelco
static int _broker_auth_info_proc_success_cb(struct emm_context_s *emm_ctx);


static int _authentication_check_imsi_5_4_2_5__1(
  struct emm_context_s *emm_context);
static int _authentication_check_imsi_5_4_2_5__1_fail(
  struct emm_context_s *emm_context);
static int _authentication_request(
  struct emm_context_s* emm_ctx,
  nas_emm_auth_proc_t* auth_proc);
static int _authentication_reject(
  struct emm_context_s *emm_context,
  struct nas_base_proc_s *base_proc);

static void _nas_itti_auth_info_req(
  const mme_ue_s1ap_id_t ue_idP,
  const imsi_t* const imsiP,
  const bool is_initial_reqP,
  plmn_t* const visited_plmnP,
  const uint8_t num_vectorsP,
  const_bstring const auts_pP);

// added for brokerd utelco
static void _nas_itti_broker_auth_info_req(
  const mme_ue_s1ap_id_t ue_id,
  const imsi_t* const imsiP,
  const bool is_initial_reqP,
  plmn_t* const visited_plmnP,
  const uint8_t num_vectorsP,
  const_bstring const auts_pP,
  const_bstring const token,
  const_bstring const ue_sig,
  const_bstring const br_id,
  EC_KEY* ut_private_ecdsa,
  RSA* ut_private_rsa);

static void _s6a_auth_info_rsp_timer_expiry_handler(void *args);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
        Authentication procedure executed by the MME
   --------------------------------------------------------------------------
*/
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication()                                 **
 **                                                                        **
 ** Description: Initiates authentication procedure to establish partial   **
 **      native EPS security context in the UE and the MME.        **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.2                           **
 **      The network initiates the authentication procedure by     **
 **      sending an AUTHENTICATION REQUEST message to the UE and   **
 **      starting the timer T3460. The AUTHENTICATION REQUEST mes- **
 **      sage contains the parameters necessary to calculate the   **
 **      authentication response.                                  **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      ksi:       NAS key set identifier                     **
 **      rand:      Random challenge number                    **
 **      autn:      Authentication token                       **
 **      success:   Callback function executed when the authen-**
 **             tication procedure successfully completes  **
 **      reject:    Callback function executed when the authen-**
 **             tication procedure fails or is rejected    **
 **      failure:   Callback function executed whener a lower  **
 **             layer failure occured before the authenti- **
 **             cation procedure comnpletes                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_ksi(
  struct emm_context_s *emm_context,
  nas_emm_specific_proc_t *const emm_specific_proc,
  ksi_t ksi,
  const uint8_t *const rand,
  const uint8_t *const autn,
  success_cb_t success,
  failure_cb_t failure)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if (
    (emm_context) && ((EMM_DEREGISTERED == emm_context->_emm_fsm_state) ||
                      (EMM_REGISTERED == emm_context->_emm_fsm_state))) {
    mme_ue_s1ap_id_t ue_id =
      PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)
        ->mme_ue_s1ap_id;
    OAILOG_INFO(
      LOG_NAS_EMM,
      "ue_id=" MME_UE_S1AP_ID_FMT
      " EMM-PROC  - Initiate Authentication KSI = %d\n",
      ue_id,
      ksi);

    nas_emm_auth_proc_t *auth_proc =
      get_nas_common_procedure_authentication(emm_context);
    if (!auth_proc) {
      auth_proc = nas_new_authentication_procedure(emm_context);
    }

    if (auth_proc) {
      if (emm_specific_proc) {
        if (EMM_SPEC_PROC_TYPE_ATTACH == emm_specific_proc->type) {
          auth_proc->is_cause_is_attach = true;
          OAILOG_DEBUG(
            LOG_NAS_EMM,
            "Auth proc cause is EMM_SPEC_PROC_TYPE_ATTACH (%d) for ue_id (%u)\n",
            emm_specific_proc->type,
            ue_id);
        } else if (EMM_SPEC_PROC_TYPE_TAU == emm_specific_proc->type) {
          auth_proc->is_cause_is_attach = false;
          OAILOG_DEBUG(
            LOG_NAS_EMM,
            "Auth proc cause is EMM_SPEC_PROC_TYPE_TAU (%d) for ue_id (%u)\n",
            emm_specific_proc->type,
            ue_id);
        }
      }
      // Set the RAND value
      auth_proc->ksi = ksi;
      if (rand) {
        memcpy(auth_proc->rand, rand, AUTH_RAND_SIZE);
      }
      // Set the authentication token
      if (autn) {
        memcpy(auth_proc->autn, autn, AUTH_AUTN_SIZE);
      }
      auth_proc->emm_cause = EMM_CAUSE_SUCCESS;
      auth_proc->retransmission_count = 0;
      auth_proc->ue_id = ue_id;
      ((nas_base_proc_t *) auth_proc)->parent =
        (nas_base_proc_t *) emm_specific_proc;
      auth_proc->emm_com_proc.emm_proc.delivered = NULL;
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state =
        emm_fsm_get_state(emm_context);
      auth_proc->emm_com_proc.emm_proc.not_delivered =
        _authentication_ll_failure;
      auth_proc->emm_com_proc.emm_proc.not_delivered_ho =
        _authentication_non_delivered_ho;
      auth_proc->emm_com_proc.emm_proc.base_proc.success_notif = success;
      auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif = failure;
      auth_proc->emm_com_proc.emm_proc.base_proc.abort = _authentication_abort;
      auth_proc->emm_com_proc.emm_proc.base_proc.fail_in =
        NULL; // only response
      auth_proc->emm_com_proc.emm_proc.base_proc.fail_out =
        _authentication_reject;
      auth_proc->emm_com_proc.emm_proc.base_proc.time_out =
        _authentication_t3460_handler;
    }

    /*
     * Send authentication request message to the UE
     */
    rc = _authentication_request(emm_context, auth_proc);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that common procedure has been initiated
       */
      emm_sap_t emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_context;
      rc = emm_sap_send(&emm_sap);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_proc_authentication(
  struct emm_context_s *emm_context,
  nas_emm_specific_proc_t *const emm_specific_proc,
  success_cb_t success,
  failure_cb_t failure)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  mme_ue_s1ap_id_t ue_id =
    PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)
      ->mme_ue_s1ap_id;
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_context);
  if (!auth_proc) {
    auth_proc = nas_new_authentication_procedure(emm_context);
  }
  if (auth_proc) {
    if (emm_specific_proc) {
      if (EMM_SPEC_PROC_TYPE_ATTACH == emm_specific_proc->type) {
        auth_proc->is_cause_is_attach = true;
      } else if (EMM_SPEC_PROC_TYPE_TAU == emm_specific_proc->type) {
        auth_proc->is_cause_is_attach = false;
      }
    }

    auth_proc->emm_cause = EMM_CAUSE_SUCCESS;
    auth_proc->retransmission_count = 0;
    auth_proc->ue_id = ue_id;
    ((nas_base_proc_t *) auth_proc)->parent =
      (nas_base_proc_t *) emm_specific_proc;
    auth_proc->emm_com_proc.emm_proc.delivered = NULL;
    auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state =
      emm_fsm_get_state(emm_context);
    auth_proc->emm_com_proc.emm_proc.not_delivered = NULL;
    auth_proc->emm_com_proc.emm_proc.not_delivered_ho = NULL;
    auth_proc->emm_com_proc.emm_proc.base_proc.success_notif = success;
    auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif = failure;
    auth_proc->emm_com_proc.emm_proc.base_proc.abort = _authentication_abort;
    auth_proc->emm_com_proc.emm_proc.base_proc.fail_in = NULL; // only response
    auth_proc->emm_com_proc.emm_proc.base_proc.fail_out =
      _authentication_reject;
    auth_proc->emm_com_proc.emm_proc.base_proc.time_out = NULL;

    bool run_auth_info_proc = false;
    if (!IS_EMM_CTXT_VALID_AUTH_VECTORS(emm_context)) {
      // Ask upper layer to fetch new security context
      nas_auth_info_proc_t *auth_info_proc =
        get_nas_cn_procedure_auth_info(emm_context);
      if (!auth_info_proc) {
        auth_info_proc = nas_new_cn_auth_info_procedure(emm_context);
      }
      if (!auth_info_proc->request_sent) {
        run_auth_info_proc = true;
      }
      rc = RETURNok;
    } else {
      ksi_t eksi = 0;
      if (emm_context->_security.eksi < KSI_NO_KEY_AVAILABLE) {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
        eksi = (emm_context->_security.eksi + 1) % (EKSI_MAX_VALUE + 1);
      }
      for (; eksi < MAX_EPS_AUTH_VECTORS; eksi++) {
        if (IS_EMM_CTXT_VALID_AUTH_VECTOR(
              emm_context, (eksi % MAX_EPS_AUTH_VECTORS))) {
          break;
        }
      }
      // eksi should always be 0
      if (!IS_EMM_CTXT_VALID_AUTH_VECTOR(
            emm_context, (eksi % MAX_EPS_AUTH_VECTORS))) {
        run_auth_info_proc = true;
      } else {
        rc = emm_proc_authentication_ksi(
          emm_context,
          emm_specific_proc,
          eksi,
          emm_context->_vector[eksi % MAX_EPS_AUTH_VECTORS].rand,
          emm_context->_vector[eksi % MAX_EPS_AUTH_VECTORS].autn,
          success,
          failure);
        OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
      }
    }
    if (run_auth_info_proc) {
      rc = _start_authentication_information_procedure(
        emm_context, auth_proc, NULL);
    }
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _start_authentication_information_procedure(
  struct emm_context_s *emm_context,
  nas_emm_auth_proc_t *const auth_proc,
  const_bstring auts)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  mme_ue_s1ap_id_t ue_id =
    PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)
      ->mme_ue_s1ap_id;
  // Ask upper layer to fetch new security context
  nas_auth_info_proc_t *auth_info_proc =
    get_nas_cn_procedure_auth_info(emm_context);
  if (!auth_info_proc) {
    auth_info_proc = nas_new_cn_auth_info_procedure(emm_context);
    auth_info_proc->request_sent = false;
  }

  auth_info_proc->cn_proc.base_proc.parent =
    &auth_proc->emm_com_proc.emm_proc.base_proc;
  auth_proc->emm_com_proc.emm_proc.base_proc.child =
    &auth_info_proc->cn_proc.base_proc;
  auth_info_proc->success_notif = _auth_info_proc_success_cb;
  auth_info_proc->failure_notif = _auth_info_proc_failure_cb;
  //added for brokerd uTelco
  auth_info_proc->broker_success_notif = _broker_auth_info_proc_success_cb;

  auth_info_proc->cn_proc.base_proc.time_out =
    _s6a_auth_info_rsp_timer_expiry_handler;
  auth_info_proc->ue_id = ue_id;
  auth_info_proc->resync = auth_info_proc->request_sent;

  plmn_t visited_plmn = {0};
  visited_plmn.mcc_digit1 = emm_context->originating_tai.mcc_digit1;
  visited_plmn.mcc_digit2 = emm_context->originating_tai.mcc_digit2;
  visited_plmn.mcc_digit3 = emm_context->originating_tai.mcc_digit3;
  visited_plmn.mnc_digit1 = emm_context->originating_tai.mnc_digit1;
  visited_plmn.mnc_digit2 = emm_context->originating_tai.mnc_digit2;
  visited_plmn.mnc_digit3 = emm_context->originating_tai.mnc_digit3;

  bool is_initial_req = !(auth_info_proc->request_sent);
  auth_info_proc->request_sent = true;
  nas_start_Ts6a_auth_info(
    auth_info_proc->ue_id,
    &auth_info_proc->timer_s6a,
    auth_info_proc->cn_proc.base_proc.time_out,
    emm_context);

  //modified for brokerd uTelco
  // _nas_itti_auth_info_req(
  //   ue_id,
  //   &emm_context->_imsi,
  //   is_initial_req,
  //   &visited_plmn,
  //   MAX_EPS_AUTH_VECTORS,
  //   auts);
  
  // added for brokerd utelco
  nas_emm_attach_proc_t *attach_proc = get_nas_specific_procedure_attach(emm_context);
  if(attach_proc && attach_proc->ies->btattachparametertoken && attach_proc->ies->btattachparameteruesig && attach_proc->ies->btattachparameterbrid) {
    OAILOG_INFO(LOG_NAS_EMM, "Initiate brokerd-uTelco authentication\n");
    emm_context->is_broker = true;
    // bt req start time
    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    _nas_itti_broker_auth_info_req(
    ue_id,
    &emm_context->_imsi,
    is_initial_req,
    &visited_plmn,
    MAX_EPS_AUTH_VECTORS,
    auts,
    attach_proc->ies->btattachparametertoken,
    attach_proc->ies->btattachparameteruesig,
    attach_proc->ies->btattachparameterbrid,
    emm_context->ut_private_ecdsa,
    emm_context->ut_private_rsa);
    // bt req end time
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    float sec_used = (end.tv_sec - start.tv_sec);
    float milli_used = (sec_used * 1e3) + (float)(end.tv_nsec - start.tv_nsec)/1e6;
    OAILOG_INFO(LOG_NAS_EMM, "Broker auth info req %f ms\n", milli_used);
  } else {
    OAILOG_INFO(LOG_NAS_EMM, "Initiate standard authentication\n");
    emm_context->is_broker = false;
    _nas_itti_auth_info_req(
    ue_id,
    &emm_context->_imsi,
    is_initial_req,
    &visited_plmn,
    MAX_EPS_AUTH_VECTORS,
    auts);
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
static int _start_authentication_information_procedure_synch(
  struct emm_context_s *emm_context,
  nas_emm_auth_proc_t *const auth_proc,
  const_bstring auts)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  // Ask upper layer to fetch new security context
  nas_auth_info_proc_t *auth_info_proc =
    get_nas_cn_procedure_auth_info(emm_context);

  AssertFatal(
    auth_info_proc == NULL,
    "auth_info_proc %p should have been cleared",
    auth_info_proc);
  if (!auth_info_proc) {
    auth_info_proc = nas_new_cn_auth_info_procedure(emm_context);
    auth_info_proc->request_sent = true;
    _start_authentication_information_procedure(emm_context, auth_proc, auts);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
}

//------------------------------------------------------------------------------
static int _auth_info_proc_success_cb(struct emm_context_s *emm_ctx)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  nas_auth_info_proc_t *auth_info_proc =
    get_nas_cn_procedure_auth_info(emm_ctx);
  mme_ue_s1ap_id_t ue_id =
    PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
  int rc = RETURNerror;

  if (auth_info_proc) {
    if (!emm_ctx) {
      OAILOG_ERROR(
        LOG_NAS_EMM,
        "EMM-PROC  - "
        "Failed to find UE id " MME_UE_S1AP_ID_FMT "\n",
        ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }

    // compute next eksi
    ksi_t eksi = 0;
    if (emm_ctx->_security.eksi < KSI_NO_KEY_AVAILABLE) {
      REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
      eksi = (emm_ctx->_security.eksi + 1) % (EKSI_MAX_VALUE + 1);
    }

    /*
     * Copy provided vector to user context
     */
    for (int i = 0; i < auth_info_proc->nb_vectors; i++) {
      AssertFatal(MAX_EPS_AUTH_VECTORS > i, " TOO many vectors");
      int destination_index = (i + eksi) % MAX_EPS_AUTH_VECTORS;
      memcpy(
        emm_ctx->_vector[destination_index].kasme,
        auth_info_proc->vector[i]->kasme,
        AUTH_KASME_SIZE);
      memcpy(
        emm_ctx->_vector[destination_index].autn,
        auth_info_proc->vector[i]->autn,
        AUTH_AUTN_SIZE);
      memcpy(
        emm_ctx->_vector[destination_index].rand,
        auth_info_proc->vector[i]->rand,
        AUTH_RAND_SIZE);
      memcpy(
        emm_ctx->_vector[destination_index].xres,
        auth_info_proc->vector[i]->xres.data,
        auth_info_proc->vector[i]->xres.size);
      emm_ctx->_vector[destination_index].xres_size =
        auth_info_proc->vector[i]->xres.size;
      OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC  - Received Vector %u:\n", i);
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received XRES ..: " XRES_FORMAT "\n",
        XRES_DISPLAY(emm_ctx->_vector[destination_index].xres));
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received RAND ..: " RAND_FORMAT "\n",
        RAND_DISPLAY(emm_ctx->_vector[destination_index].rand));
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received AUTN ..: " AUTN_FORMAT "\n",
        AUTN_DISPLAY(emm_ctx->_vector[destination_index].autn));
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received KASME .: " KASME_FORMAT " " KASME_FORMAT "\n",
        KASME_DISPLAY_1(emm_ctx->_vector[destination_index].kasme),
        KASME_DISPLAY_2(emm_ctx->_vector[destination_index].kasme));
      emm_ctx_set_attribute_valid(
        emm_ctx, EMM_CTXT_MEMBER_AUTH_VECTOR0 + destination_index);
    }

    nas_emm_auth_proc_t *auth_proc =
      get_nas_common_procedure_authentication(emm_ctx);

    if (auth_proc) {
      if (auth_info_proc->nb_vectors > 0) {
        emm_ctx_set_attribute_present(emm_ctx, EMM_CTXT_MEMBER_AUTH_VECTORS);

        for (; eksi < MAX_EPS_AUTH_VECTORS; eksi++) {
          if (IS_EMM_CTXT_VALID_AUTH_VECTOR(
                emm_ctx, (eksi % MAX_EPS_AUTH_VECTORS))) {
            break;
          }
        }
        // eksi should always be 0
        ksi_t eksi_mod = eksi % MAX_EPS_AUTH_VECTORS;
        AssertFatal(
          IS_EMM_CTXT_VALID_AUTH_VECTOR(emm_ctx, eksi_mod),
          "TODO No valid vector, should not happen");

        auth_proc->ksi = eksi;

        // re-enter previous EMM state, before re-initiating the procedure
        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        emm_sap.u.emm_reg.notify = false;
        emm_sap.u.emm_reg.free_proc = false;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);

        rc = emm_proc_authentication_ksi(
          emm_ctx,
          (nas_emm_specific_proc_t *) auth_info_proc->cn_proc.base_proc.parent,
          eksi,
          emm_ctx->_vector[eksi % MAX_EPS_AUTH_VECTORS].rand,
          emm_ctx->_vector[eksi % MAX_EPS_AUTH_VECTORS].autn,
          auth_proc->emm_com_proc.emm_proc.base_proc.success_notif,
          auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif);

        if (rc != RETURNok) {
          /*
           * Failed to initiate the authentication procedure
           */
          OAILOG_WARNING(
            LOG_NAS_EMM,
            "EMM-PROC  - "
            "Failed to initiate authentication procedure\n");
          auth_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        }
      } else {
        OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - "
          "Failed to initiate authentication procedure\n");
        auth_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        rc = RETURNerror;
      }

      nas_delete_cn_procedure(emm_ctx, &auth_info_proc->cn_proc);

      if (rc != RETURNok) {
        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        emm_sap.u.emm_reg.notify = true;
        emm_sap.u.emm_reg.free_proc = true;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);
      }
    } else {
      nas_delete_cn_procedure(emm_ctx, &auth_info_proc->cn_proc);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _auth_info_proc_failure_cb(struct emm_context_s *emm_ctx)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  nas_auth_info_proc_t *auth_info_proc =
    get_nas_cn_procedure_auth_info(emm_ctx);
  mme_ue_s1ap_id_t ue_id =
    PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
  int rc = RETURNerror;

  if (auth_info_proc) {
    nas_emm_auth_proc_t *auth_proc =
      get_nas_common_procedure_authentication(emm_ctx);

    int emm_cause = auth_info_proc->nas_cause;
    nas_delete_cn_procedure(emm_ctx, &auth_info_proc->cn_proc);

    if (auth_proc) {
      auth_proc->emm_cause = emm_cause;

      if (EMM_COMMON_PROCEDURE_INITIATED == emm_fsm_get_state(emm_ctx)) {
        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        emm_sap.u.emm_reg.notify = true;
        emm_sap.u.emm_reg.free_proc = false;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);
      } else {
        // cannot send sap event because in most cases EMM state is not EMM_COMMON_PROCEDURE_INITIATED
        // so use the callback of nas_emm_auth_proc_t
        // TODO seems bad design here, tricky.
        if (auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif) {
          emm_ctx->emm_cause = emm_cause;
          rc = (*auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif)(
            emm_ctx);
        } else {
          nas_delete_common_procedure(
            emm_ctx, (nas_emm_common_proc_t **) &auth_proc);
        }
      }
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
int emm_proc_authentication_failure(
  mme_ue_s1ap_id_t ue_id,
  int emm_cause,
  const_bstring auts)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  // Get the UE context
  ue_mm_context_t *ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id(ue_id);
  emm_context_t *emm_ctx = NULL;
  int rc = RETURNerror;

  if (!ue_mm_context) {
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - Failed to authenticate the UE " MME_UE_S1AP_ID_FMT "\n",
      ue_id);
    emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  OAILOG_INFO(
    LOG_NAS_EMM,
    "EMM-PROC  - Authentication failure (ue_id=" MME_UE_S1AP_ID_FMT
    ", cause=%d)\n",
    ue_id,
    emm_cause);
  emm_ctx = &ue_mm_context->emm_context;
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_ctx);

  if (auth_proc) {
    // Stop timer T3460
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__3);
    void *callback_args = NULL;
    nas_stop_T3460(
      ue_mm_context->mme_ue_s1ap_id, &auth_proc->T3460, callback_args);

    switch (emm_cause) {
      case EMM_CAUSE_SYNCH_FAILURE:
        /*
       * USIM has detected a mismatch in SQN.
       *  Ask for a new vector.
       */
        REQUIREMENT_3GPP_24_301(R10_5_4_2_4__3);

        auth_proc->sync_fail_count += 1;
        if (EMM_AUTHENTICATION_SYNC_FAILURE_MAX > auth_proc->sync_fail_count) {
          OAILOG_DEBUG(
            LOG_NAS_EMM,
            "EMM-PROC  - USIM has detected a mismatch in SQN Ask for new "
            "vector(s)\n");

          REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__3);
          // Pass back the current rand.
          REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__2);
          struct tagbstring resync_param;
          resync_param.data = (unsigned char *) calloc(1, RESYNC_PARAM_LENGTH);
          DevAssert(resync_param.data != NULL);
          if (resync_param.data == NULL) {
            OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
          }

          memcpy(
            resync_param.data,
            (emm_ctx->_vector[emm_ctx->_security.vector_index].rand),
            RAND_LENGTH_OCTETS);
          memcpy(
            (resync_param.data + RAND_LENGTH_OCTETS), auts->data, AUTS_LENGTH);
          // TODO: Double check this case as there is no identity request being sent.
          _start_authentication_information_procedure_synch(
            emm_ctx, auth_proc, &resync_param);
          free_wrapper((void **) &resync_param.data);
          emm_ctx_clear_auth_vectors(emm_ctx);
          rc = RETURNok;
          OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
        } else {
          REQUIREMENT_3GPP_24_301(R10_5_4_2_7_e__NOTE3);
          auth_proc->emm_cause = EMM_CAUSE_SYNCH_FAILURE;
          emm_sap_t emm_sap = {0};
          emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
          emm_sap.u.emm_reg.ue_id = ue_id;
          emm_sap.u.emm_reg.ctx = emm_ctx;
          emm_sap.u.emm_reg.notify = true;
          emm_sap.u.emm_reg.free_proc = true;
          emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
          emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
            auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
          rc = emm_sap_send(&emm_sap);
        }
        break;
      case EMM_CAUSE_MAC_FAILURE:
        REQUIREMENT_3GPP_24_301(R10_5_4_2_7_c__2);
        auth_proc->mac_fail_count++;
        auth_proc->sync_fail_count = 0;
        if (!IS_EMM_CTXT_PRESENT_IMSI(
              emm_ctx)) { // VALID means received in IDENTITY RESPONSE
          if (1 == auth_proc->mac_fail_count) {
            // Only to return to a "valid" EMM state
            {
              emm_sap_t emm_sap = {0};
              emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
              emm_sap.u.emm_reg.ue_id = ue_id;
              emm_sap.u.emm_reg.ctx = emm_ctx;
              emm_sap.u.emm_reg.notify = false;
              emm_sap.u.emm_reg.free_proc = false;
              emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
              emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
                auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
              rc = emm_sap_send(&emm_sap);
            }
            rc = emm_proc_identification(
              emm_ctx,
              &auth_proc->emm_com_proc.emm_proc,
              IDENTITY_TYPE_2_IMSI,
              _authentication_check_imsi_5_4_2_5__1,
              _authentication_check_imsi_5_4_2_5__1_fail);
          } else {
            rc = RETURNerror;
          }

          if (rc != RETURNok) {
            REQUIREMENT_3GPP_24_301(
              R10_5_4_2_7_c__NOTE1); // more or less this case...
            // Failed to initiate the identification procedure
            auth_proc->emm_cause =
              EMM_CAUSE_MAC_FAILURE; // EMM_CAUSE_ILLEGAL_UE;
            /*
           * Notify EMM that the authentication procedure failed
           */
            emm_sap_t emm_sap = {0};
            emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
            emm_sap.u.emm_reg.ue_id = ue_id;
            emm_sap.u.emm_reg.ctx = emm_ctx;
            emm_sap.u.emm_reg.notify = true;
            emm_sap.u.emm_reg.free_proc = true;
            emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
            emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
              auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
            increment_counter(
              "authentication_failure", 1, 1, "cause", "mac_failure");
            increment_counter(
              "ue_attach",
              1,
              2,
              "result",
              "failure",
              "cause",
              "authentication_mac_failure");
            rc = emm_sap_send(&emm_sap);
          }
        } else {
          REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
          auth_proc->emm_cause = EMM_CAUSE_MAC_FAILURE; //EMM_CAUSE_ILLEGAL_UE;
          // Do not accept the UE to attach to the network
          emm_sap_t emm_sap = {0};
          emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
          emm_sap.u.emm_reg.ue_id = ue_id;
          emm_sap.u.emm_reg.ctx = emm_ctx;
          emm_sap.u.emm_reg.notify = true;
          emm_sap.u.emm_reg.free_proc = true;
          emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
          emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
            auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
          increment_counter(
            "authentication_failure", 1, 1, "cause", "mac_failure");
          increment_counter(
            "ue_attach",
            1,
            2,
            "result",
            "failure",
            "cause",
            "authentication_mac_failure");
          rc = emm_sap_send(&emm_sap);
        }
        break;
      case EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE:
        increment_counter(
          "authentication_failure", 1, 1, "cause", "amf_unacceptable");
        increment_counter(
          "ue_attach",
          1,
          2,
          "result",
          "failure",
          "cause",
          "authentication_amf_failure");
        // never happened TODO check the code
        auth_proc->sync_fail_count = 0;
        REQUIREMENT_3GPP_24_301(R10_5_4_2_7_d__1);
        // test IS_EMM_CTXT_VALID_IMSI should be enough...
        if (
          (emm_ctx->is_initial_identity_imsi) ||
          (IS_EMM_CTXT_VALID_IMSI(emm_ctx))) {
          rc = RETURNerror;
        } else {
          // Only to return to a "valid" EMM state
          {
            emm_sap_t emm_sap = {0};
            emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
            emm_sap.u.emm_reg.ue_id = ue_id;
            emm_sap.u.emm_reg.ctx = emm_ctx;
            emm_sap.u.emm_reg.notify = false;
            emm_sap.u.emm_reg.free_proc = false;
            emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
            emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
              auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
            rc = emm_sap_send(&emm_sap);
          }
          if (auth_proc->unchecked_imsi) {
            free_wrapper((void **) &auth_proc->unchecked_imsi);
          }
          auth_proc->unchecked_imsi =
            calloc(1, sizeof(*auth_proc->unchecked_imsi));
          memcpy(
            auth_proc->unchecked_imsi,
            &emm_ctx->_imsi,
            sizeof(*auth_proc->unchecked_imsi));
          rc = emm_proc_identification(
            emm_ctx,
            &auth_proc->emm_com_proc.emm_proc,
            IDENTITY_TYPE_2_IMSI,
            _authentication_check_imsi_5_4_2_5__1,
            _authentication_check_imsi_5_4_2_5__1_fail);
        }
        if (rc != RETURNok) {
          REQUIREMENT_3GPP_24_301(
            R10_5_4_2_7_d__NOTE2); // more or less this case...
          // Failed to initiate the identification procedure
          OAILOG_WARNING(
            LOG_NAS_EMM,
            "ue_id=" MME_UE_S1AP_ID_FMT
            "EMM-PROC  - Failed to initiate identification procedure\n",
            ue_mm_context->mme_ue_s1ap_id);
          auth_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
          // Do not accept the UE to attach to the network
          emm_sap_t emm_sap = {0};
          emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
          emm_sap.u.emm_reg.ue_id = ue_id;
          emm_sap.u.emm_reg.ctx = emm_ctx;
          emm_sap.u.emm_reg.notify = true;
          emm_sap.u.emm_reg.free_proc = true;
          emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
          emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
            auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
          rc = emm_sap_send(&emm_sap);
        }
        break;

      default:
        auth_proc->sync_fail_count = 0;
        OAILOG_DEBUG(
          LOG_NAS_EMM,
          "EMM-PROC  - The MME received an unknown EMM CAUSE %d\n",
          emm_cause);
        OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_authentication_complete()                            **
 **                                                                        **
 ** Description: Performs the authentication completion procedure executed **
 **      by the network.                                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.4                           **
 **      Upon receiving the AUTHENTICATION RESPONSE message, the           **
 **      MME shall stop timer T3460 and check the correctness of           **
 **      the RES parameter.                                                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                          **
 **      emm_cause: Authentication failure EMM cause code                  **
 **      res:       Authentication response parameter. or auts             **
 **                 in case of sync failure                                **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    _emm_data, T3460                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_authentication_complete(
  mme_ue_s1ap_id_t ue_id,
  authentication_response_msg *msg,
  int emm_cause,
  const_bstring const res)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  int idx;
  bool is_val_fail = false;
  OAILOG_INFO(
    LOG_NAS_EMM,
    "EMM-PROC  - Authentication complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
    ue_id);

  // Get the UE context
  ue_mm_context_t *ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id(ue_id);
  emm_context_t *emm_ctx = NULL;

  if (!ue_mm_context) {
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - Failed to authenticate the UE due to NULL ue_mm_context\n");
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  OAILOG_INFO(
    LOG_NAS_EMM,
    "EMM-PROC  - Authentication complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
    ue_id);
  emm_ctx = &ue_mm_context->emm_context;
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_ctx);

  if (auth_proc) {
    // Stop timer T3460
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__1);
    void *callback_arg = NULL;
    nas_stop_T3460(ue_id, &auth_proc->T3460, callback_arg);
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
    emm_ctx_set_security_eksi(emm_ctx, auth_proc->ksi);

    for (idx = 0; idx < emm_ctx->_vector[auth_proc->ksi].xres_size; idx++) {
      if (
        (emm_ctx->_vector[auth_proc->ksi].xres[idx]) !=
        msg->authenticationresponseparameter->data[idx]) {
        is_val_fail = true;
        break;
      }
    }

    if (is_val_fail == true) {
      OAILOG_WARNING(
        LOG_NAS_EMM,
        "XRES/RES Validation Failed for (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
        ue_id);
      if (!IS_EMM_CTXT_PRESENT_IMSI(
            emm_ctx)) { // VALID means received in IDENTITY RESPONSE
        REQUIREMENT_3GPP_24_301(R10_5_4_2_7_c__2);
        rc = emm_proc_identification(
          emm_ctx,
          &auth_proc->emm_com_proc.emm_proc,
          IDENTITY_TYPE_2_IMSI,
          _authentication_check_imsi_5_4_2_5__1,
          _authentication_check_imsi_5_4_2_5__1_fail);

        if (rc != RETURNok) {
          REQUIREMENT_3GPP_24_301(
            R10_5_4_2_7_c__NOTE1); // more or less this case...
          // Failed to initiate the identification procedure
          emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
          // Do not accept the UE to attach to the network
          rc = _authentication_reject(emm_ctx, (nas_base_proc_t *) auth_proc);
        }
      } else {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        OAILOG_ERROR(
          LOG_NAS_EMM,
          "ue_id=" MME_UE_S1AP_ID_FMT "Auth Failed. XRES is not equal to RES\n",
          auth_proc->ue_id);
        increment_counter(
          "authentication_failure", 1, 1, "cause", "xres_validation_failed");
        increment_counter(
          "ue_attach",
          1,
          2,
          "result",
          "failure",
          "cause",
          "authentication_xres_validation_failed");
        // Do not accept the UE to attach to the network
        rc = _authentication_reject(emm_ctx, (nas_base_proc_t *) auth_proc);
      }
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }

    OAILOG_DEBUG(
      LOG_NAS_EMM,
      "EMM-PROC  - Successful authentication of the UE RESP XRES == XRES UE "
      "CONTEXT\n");

    /*
   * Notify EMM that the authentication procedure successfully completed
   */
    OAILOG_DEBUG(
      LOG_NAS_EMM,
      "EMM-PROC  - Notify EMM that the authentication procedure successfully "
      "completed\n");
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;

    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    rc = emm_sap_send(&emm_sap);
  } else {
    OAILOG_ERROR(LOG_NAS_EMM, "Auth proc is null");
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

// added for brokerd utelco
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_bt_authentication_complete()                            **
 **                                                                        **
 ** Description: Performs the authentication completion procedure executed **
 **      by the network.                                                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.4                           **
 **      Upon receiving the AUTHENTICATION RESPONSE message, the           **
 **      MME shall stop timer T3460 and check the correctness of           **
 **      the RES parameter.                                                **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                          **
 **      emm_cause: Authentication failure EMM cause code                  **
 **      res:       Authentication response parameter. or auts             **
 **                 in case of sync failure                                **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **      Others:    _emm_data, T3460                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_bt_authentication_complete(
  mme_ue_s1ap_id_t ue_id,
  bt_authentication_response_msg *msg,
  int emm_cause,
  const_bstring const res)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  bool is_val_fail = false;
  OAILOG_INFO(
    LOG_NAS_EMM,
    "EMM-PROC  - BT Authentication complete (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
    ue_id);

  // Get the UE context
  ue_mm_context_t *ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id(ue_id);
  emm_context_t *emm_ctx = NULL;

  if (!ue_mm_context) {
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - Failed to authenticate the UE due to NULL ue_mm_context\n");
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  emm_ctx = &ue_mm_context->emm_context;
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_ctx);

  if (auth_proc) {
    // Stop timer T3460
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__1);
    void *callback_arg = NULL;
    nas_stop_T3460(ue_id, &auth_proc->T3460, callback_arg);
    REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
    emm_ctx_set_security_eksi(emm_ctx, auth_proc->ksi);

    // for (idx = 0; idx < emm_ctx->_vector[auth_proc->ksi].xres_size; idx++) {
    //   if (
    //     (emm_ctx->_vector[auth_proc->ksi].xres[idx]) !=
    //     msg->authenticationresponseparameter->data[idx]) {
    //     is_val_fail = true;
    //     break;
    //   }
    // }
    if(blength(msg->btauthenticationresponseparameter) != 1 ||  msg->btauthenticationresponseparameter->data[0] == false)
      is_val_fail = true;

    if (is_val_fail == true) {
      OAILOG_WARNING(
        LOG_NAS_EMM,
        "XRES/RES Validation Failed for (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
        ue_id);
      if (!IS_EMM_CTXT_PRESENT_IMSI(
            emm_ctx)) { // VALID means received in IDENTITY RESPONSE
        REQUIREMENT_3GPP_24_301(R10_5_4_2_7_c__2);
        rc = emm_proc_identification(
          emm_ctx,
          &auth_proc->emm_com_proc.emm_proc,
          IDENTITY_TYPE_2_IMSI,
          _authentication_check_imsi_5_4_2_5__1,
          _authentication_check_imsi_5_4_2_5__1_fail);

        if (rc != RETURNok) {
          REQUIREMENT_3GPP_24_301(
            R10_5_4_2_7_c__NOTE1); // more or less this case...
          // Failed to initiate the identification procedure
          emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
          // Do not accept the UE to attach to the network
          rc = _authentication_reject(emm_ctx, (nas_base_proc_t *) auth_proc);
        }
      } else {
        REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
        emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        OAILOG_ERROR(
          LOG_NAS_EMM,
          "ue_id=" MME_UE_S1AP_ID_FMT "Auth Failed. XRES is not equal to RES\n",
          auth_proc->ue_id);
        increment_counter(
          "authentication_failure", 1, 1, "cause", "xres_validation_failed");
        increment_counter(
          "ue_attach",
          1,
          2,
          "result",
          "failure",
          "cause",
          "authentication_xres_validation_failed");
        // Do not accept the UE to attach to the network
        rc = _authentication_reject(emm_ctx, (nas_base_proc_t *) auth_proc);
      }
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }

    OAILOG_INFO(
      LOG_NAS_EMM,
      "EMM-PROC  - Successful BT authentication of the UE RESP true indicator\n");

    /*
   * Notify EMM that the authentication procedure successfully completed
   */
    OAILOG_INFO(
      LOG_NAS_EMM,
      "EMM-PROC  - Notify EMM that the BT authentication procedure successfully "
      "completed\n");
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;

    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    rc = emm_sap_send(&emm_sap);
  } else {
    OAILOG_ERROR(LOG_NAS_EMM, "Auth proc is null");
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/**
 * When the NAS authentication procedures are restored from data store, the
 * references to callback functions need to be re-populated with the local
 * function pointers. The functions below populate these callbacks from
 * authentication information procedure and authentication procedure.
 * The memory for each procedure is allocated by the caller
 */

void set_callbacks_for_auth_info_proc(nas_auth_info_proc_t *auth_info_proc)
{
  auth_info_proc->success_notif = _auth_info_proc_success_cb;
  auth_info_proc->failure_notif = _auth_info_proc_failure_cb;
  //added for brokerd uTelco
  auth_info_proc->broker_success_notif = _broker_auth_info_proc_success_cb;

  auth_info_proc->cn_proc.base_proc.time_out =
    _s6a_auth_info_rsp_timer_expiry_handler;
}

void set_callbacks_for_auth_proc(nas_emm_auth_proc_t *auth_proc)
{
  auth_proc->emm_com_proc.emm_proc.not_delivered =
    _authentication_ll_failure;
  auth_proc->emm_com_proc.emm_proc.not_delivered_ho =
    _authentication_non_delivered_ho;
  auth_proc->emm_com_proc.emm_proc.base_proc.abort = _authentication_abort;
  auth_proc->emm_com_proc.emm_proc.base_proc.fail_in =
    NULL;
  auth_proc->emm_com_proc.emm_proc.base_proc.fail_out =
    _authentication_reject;
  auth_proc->emm_com_proc.emm_proc.base_proc.time_out =
    _authentication_t3460_handler;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
                Timer handlers
   --------------------------------------------------------------------------
*/

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_t3460_handler()                           **
 **                                                                        **
 ** Description: T3460 timeout handler                                     **
 **      Upon T3460 timer expiration, the authentication request   **
 **      message is retransmitted and the timer restarted. When    **
 **      retransmission counter is exceed, the MME shall abort the **
 **      authentication procedure and any ongoing EMM specific     **
 **      procedure and release the NAS signalling connection.      **
 **                                                                        **
 **              3GPP TS 24.301, section 5.4.2.7, case b                   **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void _authentication_t3460_handler(void *args)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_context_t *emm_ctx = (emm_context_t *) (args);

  if (!(emm_ctx)) {
    OAILOG_ERROR(LOG_NAS_EMM, "T3460 timer expired No EMM context\n");
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_ctx);
  mme_ue_s1ap_id_t ue_id;

  if (auth_proc) {
    /*
     * Increment the retransmission counter
     */
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_b);
    // TODO the network shall abort any ongoing EMM specific procedure.

    auth_proc->retransmission_count += 1;
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - T3460 timer expired, retransmission "
      "counter = %d\n",
      auth_proc->retransmission_count);

    ue_id = auth_proc->ue_id;

    if (auth_proc->retransmission_count < AUTHENTICATION_COUNTER_MAX) {
      /*
       * Send authentication request message to the UE
       */
      _authentication_request(emm_ctx, auth_proc);
    } else {
      emm_context_t *emm_ctx = emm_context_get(&_emm_data, auth_proc->ue_id);
      /*
       * Abort the authentication and attach procedure
       */
      increment_counter("nas_auth_rsp_timer_expired", 1, NO_LABELS);
      increment_counter(
        "ue_attach",
        1,
        2,
        "result",
        "failure",
        "cause",
        "no_response_for_auth_request");
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id = auth_proc->ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      emm_sap.u.emm_reg.notify = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
        auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      emm_sap_send(&emm_sap);
      emm_common_cleanup_by_ueid(ue_id);

      // abort ANY ongoing EMM procedure (R10_5_4_2_7_b)
      nas_delete_all_emm_procedures(emm_ctx);

      // Clean up MME APP UE context
      memset((void *) &emm_sap, 0, sizeof(emm_sap));
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ue_id;
      emm_sap_send(&emm_sap);
      increment_counter("ue_attach", 1, 1, "action", "attach_abort");
    }
  }
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

static int _authentication_check_imsi_5_4_2_5__1(
  struct emm_context_s *emm_context)
{
  int rc = RETURNerror;

  if (!(emm_context)) {
    OAILOG_ERROR(LOG_NAS_EMM, "T3460 timer expired No EMM context\n");
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_context);

  if (auth_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_4_2_5__1);
    if (IS_EMM_CTXT_VALID_IMSI(
          emm_context)) { // VALID means received in IDENTITY RESPONSE
      // if IMSI are not equal
      if (memcmp(
            auth_proc->unchecked_imsi, &emm_context->_imsi, sizeof(imsi_t))) {
        // the authentication should be restarted with the correct parameters

        emm_ctx_clear_auth_vectors(emm_context);

        success_cb_t success_cb =
          auth_proc->emm_com_proc.emm_proc.base_proc.success_notif;
        failure_cb_t failure_cb =
          auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif;
        nas_emm_specific_proc_t *emm_specific_proc =
          (nas_emm_specific_proc_t *) ((nas_base_proc_t *) auth_proc)->parent;

        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
        emm_sap.u.emm_reg.ue_id = auth_proc->ue_id;
        emm_sap.u.emm_reg.ctx = emm_context;
        emm_sap.u.emm_reg.notify = true;
        emm_sap.u.emm_reg.free_proc = true;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);

        rc = emm_proc_authentication(
          emm_context, emm_specific_proc, success_cb, failure_cb);
        OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
      }
    }
    REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
    emm_sap.u.emm_reg.ue_id = auth_proc->ue_id;
    emm_sap.u.emm_reg.ctx = emm_context;
    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    rc = emm_sap_send(&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _authentication_check_imsi_5_4_2_5__1_fail(
  struct emm_context_s *emm_context)
{
  int rc = RETURNerror;
  if (!(emm_context)) {
    OAILOG_ERROR(LOG_NAS_EMM, "T3460 timer expired No EMM context\n");
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  nas_emm_auth_proc_t *auth_proc =
    get_nas_common_procedure_authentication(emm_context);

  if (auth_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_4_2_5__2);
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
    emm_sap.u.emm_reg.ue_id = auth_proc->ue_id;
    emm_sap.u.emm_reg.ctx = emm_context;
    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    rc = emm_sap_send(&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_request()                                     **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REQUEST message and start timer T3460**
 **                                                                        **
 ** Inputs:  args: pointer to emm context                                  **
 **                handler parameters                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_request(
  struct emm_context_s* emm_ctx,
  nas_emm_auth_proc_t* auth_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if (auth_proc) {
    /*
     * Notify EMM-AS SAP that Authentication Request message has to be sent
     * to the UE
     */
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMAS_SECURITY_REQ;
    emm_sap.u.emm_as.u.security.puid =
      auth_proc->emm_com_proc.emm_proc.base_proc.nas_puid;
    emm_sap.u.emm_as.u.security.guti = NULL;
    emm_sap.u.emm_as.u.security.ue_id = auth_proc->ue_id;
    emm_sap.u.emm_as.u.security.ksi = auth_proc->ksi;
    if (auth_proc->is_broker) {
      // added for brokerd uTelco
      emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_BT_AUTH;
      memcpy(emm_sap.u.emm_as.u.security.br_ue_token, auth_proc->br_ue_token, BR_UE_TOKEN_SIZE);
      memcpy(emm_sap.u.emm_as.u.security.br_ue_token_br_sig, auth_proc->br_ue_token_br_sig, BR_UE_TOKEN_BR_SIG_SIZE);
    }
    else {
      emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_AUTH;
      memcpy(emm_sap.u.emm_as.u.security.rand, auth_proc->rand, AUTH_RAND_SIZE);
      memcpy(emm_sap.u.emm_as.u.security.autn, auth_proc->autn, AUTH_AUTN_SIZE);
    }

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data(
      &emm_sap.u.emm_as.u.security.sctx, &emm_ctx->_security, false, true);
    REQUIREMENT_3GPP_24_301(R10_5_4_2_2);
    rc = emm_sap_send(&emm_sap);

    if (rc != RETURNerror) {
      if (emm_ctx) {
        if (auth_proc->T3460.id != NAS_TIMER_INACTIVE_ID) {
          void *timer_callback_args = NULL;
          nas_stop_T3460(
            auth_proc->ue_id, &auth_proc->T3460, timer_callback_args);
        }
        /*
         * Start T3460 timer
         */
        nas_start_T3460(
          auth_proc->ue_id,
          &auth_proc->T3460,
          auth_proc->emm_com_proc.emm_proc.base_proc.time_out,
          (void *) emm_ctx);
      }
    }
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_reject()                                  **
 **                                                                        **
 ** Description: Sends AUTHENTICATION REJECT message                       **
 **                                                                        **
 ** Inputs:  ue_id:      UE lower layer identifier                  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _authentication_reject(
  emm_context_t *emm_context,
  struct nas_base_proc_s *base_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_sap_t emm_sap = {0};
  int rc = RETURNerror;
  if ((base_proc) && (emm_context)) {
    nas_emm_auth_proc_t *auth_proc = (nas_emm_auth_proc_t *) base_proc;

    /*
     * Notify EMM-AS SAP that Authentication Reject message has to be sent
     * to the UE
     */
    emm_sap.primitive = EMMAS_SECURITY_REJ;
    emm_sap.u.emm_as.u.security.guti = NULL;
    emm_sap.u.emm_as.u.security.ue_id = auth_proc->ue_id;
    increment_counter("ue_attach", 1, 1, "action", "auth_reject_sent");
    emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_AUTH;

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data(
      &emm_sap.u.emm_as.u.security.sctx, &emm_context->_security, false, true);
    rc = emm_sap_send(&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_ll_failure()                                   **
 **                                                                        **
 ** Description: Aborts the authentication procedure currently in progress **
 ** Inputs:  args:      Authentication data to be released         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3460                                      **
 **                                                                        **
 ***************************************************************************/
static int _authentication_ll_failure(
  struct emm_context_s *emm_context,
  struct nas_emm_proc_s *emm_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  if ((emm_proc) && (emm_context)) {
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_a);
    nas_emm_auth_proc_t *auth_proc = (nas_emm_auth_proc_t *) emm_proc;
    emm_sap_t emm_sap = {0};

    emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = auth_proc->ue_id;
    emm_sap.u.emm_reg.ctx = emm_context;
    emm_sap.u.emm_reg.notify = true;
    emm_sap.u.emm_reg.free_proc = true;
    emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    rc = emm_sap_send(&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_non_delivered()                               **
 **                                                                        **
 ** Description:                                                           **
 ** Inputs:  args:      Authentication data to be released                 **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                                  **
 **                                                                        **
 ***************************************************************************/
static int _authentication_non_delivered_ho(
  struct emm_context_s *emm_ctx,
  struct nas_emm_proc_s *emm_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if ((emm_proc) && (emm_ctx)) {
    ue_mm_context_t *ue_mm_context =
      PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context);
    nas_emm_auth_proc_t *auth_proc = (nas_emm_auth_proc_t *) emm_proc;
    REQUIREMENT_3GPP_24_301(R10_5_4_2_7_j);
    mme_ue_s1ap_id_t ue_id = auth_proc->ue_id;
    /************************README***********************************************
  ** NAS Non Delivery indication during HO handling will be added when HO is
  ** supported.
  ** In non hand-over case if MME receives NAS Non Delivery indication message
  ** that implies eNB and UE has lost radio connection. In this case aborting
  ** the Authentication and Attach Procedure.
  *****************************************************************************
  REQUIREMENT_3GPP_24_301(R10_5_4_2_7_j);
  ****************************************************************************/
    /*
       * Stop timer T3460
       */
    if (auth_proc->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO(
        LOG_NAS_EMM,
        "EMM-PROC  - Stop timer T3460 (%ld)\n",
        auth_proc->T3460.id);
      nas_stop_T3460(ue_mm_context->mme_ue_s1ap_id, &auth_proc->T3460, NULL);
    }
    /*
       * Abort authentication and attach procedure
       */
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
    emm_sap.u.emm_reg.ue_id = ue_id;
    emm_sap.u.emm_reg.ctx = emm_ctx;
    emm_sap_send(&emm_sap);
    emm_common_cleanup_by_ueid(ue_id);
    // Clean up MME APP UE context
    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ue_id;
    rc = emm_sap_send(&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _authentication_abort()                                       **
 **                                                                        **
 ** Description: Aborts the authentication procedure currently in progress **
 **                                                                        **
 ** Inputs:  args:      Authentication data to be released                 **
 **      Others:    None                                                   **
 **                                                                        **
 ** Outputs:     None
 **     Return: None
 **     Others: None
 **                                                                        **
 ***************************************************************************/
static int _authentication_abort(
  emm_context_t *emm_ctx,
  struct nas_base_proc_s *base_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  if ((base_proc) && (emm_ctx)) {
    nas_emm_auth_proc_t *auth_proc = (nas_emm_auth_proc_t *) base_proc;
    ue_mm_context_t *ue_mm_context =
      PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context);
    OAILOG_INFO(
      LOG_NAS_EMM,
      "EMM-PROC  - Abort authentication procedure "
      "(ue_id=" MME_UE_S1AP_ID_FMT ")\n",
      ue_mm_context->mme_ue_s1ap_id);

    /*
     * Stop timer T3460
     */
    void *timer_callback_args = NULL;
    nas_stop_T3460(
      ue_mm_context->mme_ue_s1ap_id, &auth_proc->T3460, timer_callback_args);
    rc = RETURNok;
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_itti_auth_info_req()                                     **
 **                                                                        **
 ** Description: Sends Authenticatio Req to HSS via S6a Task               **
 **                                                                        **
 ** Inputs: ue_idP: UE context Identifier                                  **
 **      imsiP: IMSI of UE                                                 **
 **      is_initial_reqP: Flag to indicate, whether Auth Req is sent       **
 **                      for first time or initited as part of             **
 **                      re-synchronisation                                **
 **      visited_plmnP : Visited PLMN                                      **
 **      num_vectorsP : Number of Auth vectors in case of                  **
 **                    re-synchronisation                                  **
 **      auts_pP : sent in case of re-synchronisation                      **
 ** Outputs:                                                               **
 **     Return: None                                                       **
 **                                                                        **
 ***************************************************************************/
static void _nas_itti_auth_info_req(
  const mme_ue_s1ap_id_t ue_id,
  const imsi_t* const imsiP,
  const bool is_initial_reqP,
  plmn_t* const visited_plmnP,
  const uint8_t num_vectorsP,
  const_bstring const auts_pP)
{
  OAILOG_FUNC_IN(LOG_NAS);
  MessageDef* message_p = NULL;
  s6a_auth_info_req_t* auth_info_req = NULL;

  OAILOG_INFO(
    LOG_NAS_EMM,
    "Sending Authentication Information Request message to S6A"
    " for ue_id =" MME_UE_S1AP_ID_FMT "\n",
    ue_id);

  message_p = itti_alloc_new_message(TASK_MME_APP, S6A_AUTH_INFO_REQ);

  if (!message_p) {
    OAILOG_CRITICAL(
      LOG_NAS_EMM,
      "itti_alloc_new_message failed for Authentication"
      " Information Request message to S6A for"
      " ue-id = " MME_UE_S1AP_ID_FMT "\n",
      ue_id);
    OAILOG_FUNC_OUT(LOG_NAS);
  }
  auth_info_req = &message_p->ittiMsg.s6a_auth_info_req;
  memset(auth_info_req, 0, sizeof(s6a_auth_info_req_t));

  IMSI_TO_STRING(imsiP, auth_info_req->imsi, IMSI_BCD_DIGITS_MAX + 1);
  auth_info_req->imsi_length = (uint8_t) strlen(auth_info_req->imsi);

  if (!(auth_info_req->imsi_length > 5) && (auth_info_req->imsi_length < 16)) {
    OAILOG_WARNING(
      LOG_NAS_EMM, "Bad IMSI length %d", auth_info_req->imsi_length);
    OAILOG_FUNC_OUT(LOG_NAS);
  }
  auth_info_req->visited_plmn = *visited_plmnP;
  auth_info_req->nb_of_vectors = num_vectorsP;

  if (is_initial_reqP) {
    auth_info_req->re_synchronization = 0;
    memset(auth_info_req->resync_param, 0, sizeof auth_info_req->resync_param);
  } else {
    if (!auts_pP) {
      OAILOG_WARNING(LOG_NAS_EMM, "Auts Null during resynchronization \n");
      OAILOG_FUNC_OUT(LOG_NAS);
    }
    auth_info_req->re_synchronization = 1;
    memcpy(
      auth_info_req->resync_param,
      auts_pP->data,
      sizeof auth_info_req->resync_param);
  }
  itti_send_msg_to_task(TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

// added for brokerd utelco
/****************************************************************************
 **                                                                        **
 ** Name:    _nas_itti_broker_auth_info_req()                                     **
 **                                                                        **
 ** Description: Sends Broker Authenticatio Req to HSS via S6a Task               **
 **                                                                        **
 ** Inputs: ue_idP: UE context Identifier                                  **
 **      imsiP: IMSI of UE                                                 **
 **      is_initial_reqP: Flag to indicate, whether Auth Req is sent       **
 **                      for first time or initited as part of             **
 **                      re-synchronisation                                **
 **      visited_plmnP : Visited PLMN                                      **
 **      num_vectorsP : Number of Auth vectors in case of                  **
 **                    re-synchronisation                                  **
 **      auts_pP : sent in case of re-synchronisation                      **
 ** Outputs:                                                               **
 **     Return: None                                                       **
 **                                                                        **
 ***************************************************************************/
static void _nas_itti_broker_auth_info_req(
  const mme_ue_s1ap_id_t ue_id,
  const imsi_t* const imsiP,
  const bool is_initial_reqP,
  plmn_t* const visited_plmnP,
  const uint8_t num_vectorsP,
  const_bstring const auts_pP,
  const_bstring const ue_br_token,
  const_bstring const ue_br_token_ue_sig,
  const_bstring const br_id,
  EC_KEY  * ut_private_ecdsa,
  RSA* ut_private_rsa)
{
  OAILOG_FUNC_IN(LOG_NAS);

  OAILOG_INFO(LOG_NAS_EMM,"EMM-PROC  - decode the BR Id\n");
  uint8_t plain_br_id[BR_ID_SIZE + NONCE_SIZE];  
  memcpy(plain_br_id, (unsigned char *)br_id->data, BR_ID_SIZE + NONCE_SIZE);

  if(plain_br_id[0] != 0) {
    OAILOG_CRITICAL(
      LOG_NAS_EMM,
      "Extracted a wrong broker id %d\n", plain_br_id[0]);
    OAILOG_FUNC_OUT(LOG_NAS);
  }

  MessageDef* message_p = NULL;
  broker_auth_info_req_t* auth_info_req = NULL;

  OAILOG_INFO(
    LOG_NAS_EMM,
    "Sending Broker Authentication Information Request message to S6A"
    " for ue_id =" MME_UE_S1AP_ID_FMT "\n",
    ue_id);

  message_p = itti_alloc_new_message(TASK_MME_APP, BROKER_AUTH_INFO_REQ);

  if (!message_p) {
    OAILOG_CRITICAL(
      LOG_NAS_EMM,
      "itti_alloc_new_message failed for Broker Authentication"
      " Information Request message to S6A for"
      " ue-id = " MME_UE_S1AP_ID_FMT "\n",
      ue_id);
    OAILOG_FUNC_OUT(LOG_NAS);
  }
  auth_info_req = &message_p->ittiMsg.broker_auth_info_req;
  memset(auth_info_req, 0, sizeof(broker_auth_info_req_t));

  IMSI_TO_STRING(imsiP, auth_info_req->imsi, IMSI_BCD_DIGITS_MAX + 1);
  auth_info_req->imsi_length = (uint8_t) strlen(auth_info_req->imsi);

  if (!(auth_info_req->imsi_length > 5) && (auth_info_req->imsi_length < 16)) {
    OAILOG_WARNING(
      LOG_NAS_EMM, "Bad IMSI length %d", auth_info_req->imsi_length);
    OAILOG_FUNC_OUT(LOG_NAS);
  }
  auth_info_req->visited_plmn = *visited_plmnP;
  auth_info_req->nb_of_vectors = num_vectorsP;

  if (is_initial_reqP) {
    auth_info_req->re_synchronization = 0;
    memset(auth_info_req->resync_param, 0, sizeof auth_info_req->resync_param);
  } else {
    if (!auts_pP) {
      OAILOG_WARNING(LOG_NAS_EMM, "Auts Null during resynchronization \n");
      OAILOG_FUNC_OUT(LOG_NAS);
    }
    auth_info_req->re_synchronization = 1;
    memcpy(
      auth_info_req->resync_param,
      auts_pP->data,
      sizeof auth_info_req->resync_param);
  }
  int token_length = blength(ue_br_token);
  int ue_sig_length = blength(ue_br_token_ue_sig);
  if(token_length != TOKEN_LENGTH || ue_sig_length > UE_SIGNATURE_LENGTH) {
    OAILOG_WARNING(LOG_NAS_EMM, "Bad Token length %d or Bad UE signature length %d \n", token_length, ue_sig_length);
    OAILOG_FUNC_OUT(LOG_NAS);
  }
  memcpy(auth_info_req->ue_br_token, ue_br_token->data, token_length);
  memcpy(auth_info_req->ue_br_token_ue_sig, ue_br_token_ue_sig->data, ue_sig_length);

  uint8_t payload[TOKEN_LENGTH + UE_SIGNATURE_LENGTH];
  memcpy(payload, ue_br_token->data, token_length);
  memcpy(payload + token_length, ue_br_token_ue_sig->data, ue_sig_length);
  unsigned int sig_length;
  uint8_t digest[SHA_DIGEST_LENGTH];
  SHA1(payload, token_length + ue_sig_length, digest);
  ECDSA_sign(NID_sha1, digest, SHA_DIGEST_LENGTH, (unsigned char*) auth_info_req->ue_br_token_ut_sig, &sig_length, ut_private_ecdsa);

  itti_send_msg_to_task(TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}


/************************************************************************
 **                                                                    **
 ** Name:    _s6a_auth_info_rsp_timer_expiry_handler                    **
 **                                                                    **
 ** Description:                                                       **
 **      The timer is used for monitoring Auth Response from HSS       **
 **      On expiry, MME didn't get the auth vectors from HSS, so       **
 **      MME shall sends Attach Reject to UE                           **
 **                                                                    **
 ** Inputs:  args:      handler parameters                             **
 **                                                                    **
 ************************************************************************/
static void _s6a_auth_info_rsp_timer_expiry_handler(void* args)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_context_t* emm_ctx = (emm_context_t *) (args);

  if (emm_ctx) {
    nas_auth_info_proc_t* auth_info_proc =
      get_nas_cn_procedure_auth_info(emm_ctx);
    if (!auth_info_proc) {
      OAILOG_FUNC_OUT(LOG_NAS_EMM);
    }

    void* timer_callback_args = NULL;
    nas_stop_Ts6a_auth_info(
      auth_info_proc->ue_id, &auth_info_proc->timer_s6a, timer_callback_args);

    auth_info_proc->timer_s6a.id = NAS_TIMER_INACTIVE_ID;
    if (auth_info_proc->resync) {
      OAILOG_ERROR(
        LOG_NAS_EMM,
        "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Resync auth "
        "procedure was in progress. Aborting attach procedure. UE "
        "id " MME_UE_S1AP_ID_FMT "\n",
        auth_info_proc->ue_id);
    } else {
      OAILOG_ERROR(
        LOG_NAS_EMM,
        "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Initial auth "
        "procedure was in progress. Aborting attach procedure. UE "
        "id " MME_UE_S1AP_ID_FMT "\n",
        auth_info_proc->ue_id);
    }

    // Send Attach Reject with cause NETWORK FAILURE and delete UE context
    nas_proc_auth_param_fail(auth_info_proc->ue_id, NAS_CAUSE_NETWORK_FAILURE);
  } else {
    OAILOG_ERROR(
      LOG_NAS_EMM,
      "EMM-PROC  - Timer timer_s6_auth_info_rsp expired. Null EMM Context for "
      "UE \n");
  }
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

//------------------------------------------------------------------------------
// added for broked uTelco

static int _get_sig_len(uint8_t br_ut_token_br_sig[BR_UT_TOKEN_BR_SIG_SIZE])
{
  return (int)br_ut_token_br_sig[1] + 2;
}
static int _verify_br_ut_token(uint8_t br_ut_token[BR_UT_TOKEN_SIZE], uint8_t br_ut_token_br_sig[BR_UT_TOKEN_BR_SIG_SIZE], RSA* ut_private_rsa, int br_id, EC_KEY* br_public_ecdsa)
{
  uint8_t digest[SHA_DIGEST_LENGTH];
  SHA1(br_ut_token, BR_UT_TOKEN_SIZE, digest);
  if(ECDSA_verify(NID_sha1, digest, SHA_DIGEST_LENGTH, (unsigned char *)br_ut_token_br_sig, _get_sig_len(br_ut_token_br_sig), br_public_ecdsa) == 1)
    return RETURNok;
  return RETURNerror;
}
// added for brokerd-uTelco
int emm_proc_broker_authentication_ksi(
  struct emm_context_s *emm_context,
  nas_emm_specific_proc_t *const emm_specific_proc,
  ksi_t ksi,
  const uint8_t *const br_ue_token,
  const uint8_t *const br_ue_token_br_sig,
  EC_KEY  * ut_private_ecdsa,
  success_cb_t success,
  failure_cb_t failure)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if (
    (emm_context) && ((EMM_DEREGISTERED == emm_context->_emm_fsm_state) ||
                      (EMM_REGISTERED == emm_context->_emm_fsm_state))) {
    mme_ue_s1ap_id_t ue_id =
      PARENT_STRUCT(emm_context, struct ue_mm_context_s, emm_context)
        ->mme_ue_s1ap_id;
    OAILOG_INFO(
      LOG_NAS_EMM,
      "ue_id=" MME_UE_S1AP_ID_FMT
      " EMM-PROC  - Initiate Broker Authentication KSI = %d\n",
      ue_id,
      ksi);

    nas_emm_auth_proc_t *auth_proc =
      get_nas_common_procedure_authentication(emm_context);
    if (!auth_proc) {
      auth_proc = nas_new_authentication_procedure(emm_context);
    }

    if (auth_proc) {
      if (emm_specific_proc) {
        if (EMM_SPEC_PROC_TYPE_ATTACH == emm_specific_proc->type) {
          auth_proc->is_cause_is_attach = true;
          OAILOG_DEBUG(
            LOG_NAS_EMM,
            "Auth proc cause is EMM_SPEC_PROC_TYPE_ATTACH (%d) for ue_id (%u)\n",
            emm_specific_proc->type,
            ue_id);
        } else if (EMM_SPEC_PROC_TYPE_TAU == emm_specific_proc->type) {
          auth_proc->is_cause_is_attach = false;
          OAILOG_DEBUG(
            LOG_NAS_EMM,
            "Auth proc cause is EMM_SPEC_PROC_TYPE_TAU (%d) for ue_id (%u)\n",
            emm_specific_proc->type,
            ue_id);
        }
      }
      auth_proc->ksi = ksi;
      if (br_ue_token) {
        auth_proc->is_broker = true;
        memcpy(auth_proc->br_ue_token, br_ue_token, BR_UE_TOKEN_SIZE);
        memcpy(auth_proc->br_ue_token_br_sig, br_ue_token_br_sig, BR_UE_TOKEN_BR_SIG_SIZE);
        // combine the token and broker's sig   
        uint8_t payload[BR_UE_TOKEN_SIZE + BR_UE_TOKEN_BR_SIG_SIZE];
        memcpy(payload, br_ue_token, BR_UE_TOKEN_SIZE);
        memcpy(payload + BR_UE_TOKEN_SIZE, br_ue_token_br_sig, BR_UE_TOKEN_BR_SIG_SIZE);
      }
      auth_proc->emm_cause = EMM_CAUSE_SUCCESS;
      auth_proc->retransmission_count = 0;
      auth_proc->ue_id = ue_id;
      ((nas_base_proc_t *) auth_proc)->parent =
        (nas_base_proc_t *) emm_specific_proc;
      auth_proc->emm_com_proc.emm_proc.delivered = NULL;
      auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state =
        emm_fsm_get_state(emm_context);
      auth_proc->emm_com_proc.emm_proc.not_delivered =
        _authentication_ll_failure;
      auth_proc->emm_com_proc.emm_proc.not_delivered_ho =
        _authentication_non_delivered_ho;
      auth_proc->emm_com_proc.emm_proc.base_proc.success_notif = success;
      auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif = failure;
      auth_proc->emm_com_proc.emm_proc.base_proc.abort = _authentication_abort;
      auth_proc->emm_com_proc.emm_proc.base_proc.fail_in =
        NULL; // only response
      auth_proc->emm_com_proc.emm_proc.base_proc.fail_out =
        _authentication_reject;
      auth_proc->emm_com_proc.emm_proc.base_proc.time_out =
        _authentication_t3460_handler;
    }

    /*
     * Send authentication request message to the UE
     */
    rc = _authentication_request(emm_context, auth_proc);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that common procedure has been initiated
       */
      emm_sap_t emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_context;
      rc = emm_sap_send(&emm_sap);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}
// added for brokerd uTelco
// auth infor success callback for broker
static int _broker_auth_info_proc_success_cb(struct emm_context_s *emm_ctx)
{
  // time it
  struct timespec start, end;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

  OAILOG_FUNC_IN(LOG_NAS_EMM);
  OAILOG_INFO(LOG_NAS_EMM, "Broker authentication info proc success callback");
  nas_auth_info_proc_t *auth_info_proc =
    get_nas_cn_procedure_auth_info(emm_ctx);
  mme_ue_s1ap_id_t ue_id =
    PARENT_STRUCT(emm_ctx, struct ue_mm_context_s, emm_context)->mme_ue_s1ap_id;
  int rc = RETURNerror;

  if (auth_info_proc) {
    if (!emm_ctx) {
      OAILOG_ERROR(
        LOG_NAS_EMM,
        "EMM-PROC  - "
        "Failed to find UE id " MME_UE_S1AP_ID_FMT "\n",
        ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }

    // compute next eksi
    ksi_t eksi = 0;
    if (emm_ctx->_security.eksi < KSI_NO_KEY_AVAILABLE) {
      REQUIREMENT_3GPP_24_301(R10_5_4_2_4__2);
      eksi = (emm_ctx->_security.eksi + 1) % (EKSI_MAX_VALUE + 1);
    }

    /*
     * Copy provided vector to user context
     */
    for (int i = 0; i < auth_info_proc->nb_vectors; i++) {
      AssertFatal(MAX_EPS_AUTH_VECTORS > i, " TOO many vectors");
      int destination_index = (i + eksi) % MAX_EPS_AUTH_VECTORS;
      memcpy(
        emm_ctx->_broker_vector[destination_index].br_ut_token,
        auth_info_proc->broker_vector[i]->br_ut_token,
        BR_UT_TOKEN_SIZE);
      memcpy(
        emm_ctx->_broker_vector[destination_index].br_ue_token,
        auth_info_proc->broker_vector[i]->br_ue_token,
        BR_UE_TOKEN_SIZE);
      memcpy(
        emm_ctx->_broker_vector[destination_index].br_ut_token_br_sig,
        auth_info_proc->broker_vector[i]->br_ut_token_br_sig,
        BR_UT_TOKEN_BR_SIG_SIZE);
      memcpy(
        emm_ctx->_broker_vector[destination_index].br_ue_token_br_sig,
        auth_info_proc->broker_vector[i]->br_ue_token_br_sig,
        BR_UE_TOKEN_BR_SIG_SIZE);
      OAILOG_DEBUG(LOG_NAS_EMM, "EMM-PROC  - Received Broker Vector %u:\n", i);
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received Broker BR_UT_TOKEN ..: " BR_UT_TOKEN_FORMAT "\n",
        BR_UT_TOKEN_DISPLAY(emm_ctx->_broker_vector[destination_index].br_ut_token));
      OAILOG_DEBUG(
        LOG_NAS_EMM,
        "EMM-PROC  - Received Broker BR_UE_TOKEN ..: " BR_UE_TOKEN_FORMAT "\n",
        BR_UE_TOKEN_DISPLAY(emm_ctx->_broker_vector[destination_index].br_ue_token));
      emm_ctx_set_attribute_valid(
        emm_ctx, EMM_CTXT_MEMBER_AUTH_VECTOR0 + destination_index);
    }

    nas_emm_auth_proc_t *auth_proc =
      get_nas_common_procedure_authentication(emm_ctx);

    if (auth_proc) {
      if (auth_info_proc->nb_vectors > 0) {
        emm_ctx_set_attribute_present(emm_ctx, EMM_CTXT_MEMBER_AUTH_VECTORS);

        for (; eksi < MAX_EPS_AUTH_VECTORS; eksi++) {
          if (IS_EMM_CTXT_VALID_AUTH_VECTOR(
                emm_ctx, (eksi % MAX_EPS_AUTH_VECTORS))) {
            break;
          }
        }
        // eksi should always be 0
        ksi_t eksi_mod = eksi % MAX_EPS_AUTH_VECTORS;
        AssertFatal(
          IS_EMM_CTXT_VALID_AUTH_VECTOR(emm_ctx, eksi_mod),
          "TODO No valid vector, should not happen");

        auth_proc->ksi = eksi;

        // re-enter previous EMM state, before re-initiating the procedure
        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        emm_sap.u.emm_reg.notify = false;
        emm_sap.u.emm_reg.free_proc = false;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);

        rc = _verify_br_ut_token(emm_ctx->_broker_vector[eksi % MAX_EPS_AUTH_VECTORS].br_ut_token, emm_ctx->_broker_vector[eksi % MAX_EPS_AUTH_VECTORS].br_ut_token_br_sig, emm_ctx->ut_private_rsa, emm_ctx->br_id, emm_ctx->br_public_ecdsa); 

        if(rc != RETURNok) {
          OAILOG_WARNING(
            LOG_NAS_EMM,
            "EMM-PROC  - Failed to verify BR UT Token\n");
        }
        else {
          OAILOG_INFO(
            LOG_NAS_EMM,
            "EMM-PROC  - Successfully verify BR UT Token\n");
        }

        if(rc == RETURNok) {
           // TODO: this only works for single toekn response
           OAILOG_INFO(LOG_NAS_EMM,"EMM-PROC  - decode the BR UT Token and load the Kasme\n");
           uint8_t plain_token[BR_UT_PLAIN_TOKEN_SIZE];  
           int plain_token_length = RSA_private_decrypt(BR_UT_TOKEN_SIZE, (unsigned char *)emm_ctx->_broker_vector[eksi % MAX_EPS_AUTH_VECTORS].br_ut_token, (unsigned char *)plain_token, emm_ctx->ut_private_rsa, RSA_PKCS1_PADDING);  
           if(AUTH_KASME_SIZE == UE_UT_KEY_SIZE) {
              memcpy(emm_ctx->_vector[eksi % MAX_EPS_AUTH_VECTORS].kasme, plain_token + BR_ID_SIZE, AUTH_KASME_SIZE);
           } else {
              OAILOG_WARNING(LOG_NAS_EMM,"EMM-PROC  - Inconsitent key size between BR UT Token and Kasme\n");
           }
           int sub_len = plain_token_length - (BR_ID_SIZE + UE_UT_KEY_SIZE + UT_BR_KEY_SIZE);
           char *sub_data = malloc(sizeof(char) * (sub_len + 1));
           memcpy(sub_data, plain_token + BR_ID_SIZE + UE_UT_KEY_SIZE + UT_BR_KEY_SIZE, sub_len);
           OAILOG_WARNING(LOG_NAS_EMM,"EMM-PROC  - sub data length %d and strlen %u\n", sub_len, (unsigned int)strlen(sub_data));
           subscriber_add_sub(sub_data, sub_len); // make a async RPC call to add subscriber (TODO: move it off-path)
           free(sub_data);
        }

        rc = emm_proc_broker_authentication_ksi(
          emm_ctx,
          (nas_emm_specific_proc_t *) auth_info_proc->cn_proc.base_proc.parent,
          eksi,
          emm_ctx->_broker_vector[eksi % MAX_EPS_AUTH_VECTORS].br_ue_token,
          emm_ctx->_broker_vector[eksi % MAX_EPS_AUTH_VECTORS].br_ue_token_br_sig,
          emm_ctx->ut_private_ecdsa,
          auth_proc->emm_com_proc.emm_proc.base_proc.success_notif,
          auth_proc->emm_com_proc.emm_proc.base_proc.failure_notif);

        if (rc != RETURNok) {
          /*
           * Failed to initiate the authentication procedure
           */
          OAILOG_WARNING(
            LOG_NAS_EMM,
            "EMM-PROC  - "
            "Failed to initiate authentication procedure\n");
          auth_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        }
      } else {
        OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - "
          "Failed to initiate authentication procedure\n");
        auth_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
        rc = RETURNerror;
      }

      nas_delete_cn_procedure(emm_ctx, &auth_info_proc->cn_proc);

      if (rc != RETURNok) {
        emm_sap_t emm_sap = {0};
        emm_sap.primitive = EMMREG_COMMON_PROC_REJ;
        emm_sap.u.emm_reg.ue_id = ue_id;
        emm_sap.u.emm_reg.ctx = emm_ctx;
        emm_sap.u.emm_reg.notify = true;
        emm_sap.u.emm_reg.free_proc = true;
        emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
        emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
        rc = emm_sap_send(&emm_sap);
      }
    } else {
      nas_delete_cn_procedure(emm_ctx, &auth_info_proc->cn_proc);
    }
  }
  // time the end
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  float sec_used = (end.tv_sec - start.tv_sec);
  float milli_used = (sec_used * 1e3) + (float)(end.tv_nsec - start.tv_nsec)/1e6;
  OAILOG_INFO(LOG_NAS_EMM, "Broker auth info success cb %f ms\n", milli_used);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}
