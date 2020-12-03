#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log.h"
#include "assertions.h"
#include "3gpp_requirements_24.301.h"
#include "common_types.h"
#include "3gpp_24.008.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "common_defs.h"
#include "nas_timer.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "secu_defs.h"
#include "service303.h"
#include "EmmCommon.h"
#include "3gpp_23.003.h"
#include "3gpp_24.301.h"
#include "3gpp_33.401.h"
#include "3gpp_36.401.h"
#include "emm_asDef.h"
#include "emm_cnDef.h"
#include "emm_fsm.h"
#include "emm_regDef.h"
#include "mme_api.h"
#include "mme_app_state.h"
#include "nas_procedures.h"
#include "nas/securityDef.h"
#include "security_types.h"
#include "mme_app_defs.h"

static void _usage_report_t3460_handler(void *);

static int _usage_report_abort(
  emm_context_t *emm_context,
  struct nas_base_proc_s *base_proc);

static int _usage_report_request(nas_emm_usage_report_proc_t *const ur_proc);

int emm_proc_usage_report(
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

  nas_emm_usage_report_proc_t *ur_proc =
    get_nas_common_procedure_usage_report(emm_context); 
  if (!ur_proc) {
    ur_proc = nas_new_usage_report_procedure(emm_context); 
  }

  if (ur_proc) {
    ur_proc->ue_id = ue_id;

    ur_proc->retransmission_count = 0;

    ur_proc->report_id = (unsigned) rand();

    ((nas_base_proc_t *) ur_proc)->parent =
      (nas_base_proc_t *) emm_specific_proc;

    ur_proc->emm_com_proc.emm_proc.delivered = NULL;
    ur_proc->emm_com_proc.emm_proc.previous_emm_fsm_state =
      emm_fsm_get_state(emm_context);
    ur_proc->emm_com_proc.emm_proc.not_delivered = NULL;
    ur_proc->emm_com_proc.emm_proc.not_delivered_ho = NULL;
    ur_proc->emm_com_proc.emm_proc.base_proc.success_notif = success;
    ur_proc->emm_com_proc.emm_proc.base_proc.failure_notif = failure;
    ur_proc->emm_com_proc.emm_proc.base_proc.abort = _usage_report_abort; 
    ur_proc->emm_com_proc.emm_proc.base_proc.fail_in = NULL; // only response
    ur_proc->emm_com_proc.emm_proc.base_proc.fail_out = NULL;
    ur_proc->emm_com_proc.emm_proc.base_proc.time_out = _usage_report_t3460_handler; 
    /*
     * Send Usage Report Request message to the UE
     */
    rc = _usage_report_request(ur_proc); 

    if (rc != RETURNerror) {
      /*
       * Notify EMM that common procedure has been initiated
       */
      emm_sap_t emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_context;
      emm_sap.u.emm_reg.u.common.common_proc = &ur_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
        ur_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      rc = emm_sap_send(&emm_sap);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}
//------------------------------------------------------------------------------
int emm_proc_usage_report_response(
  mme_ue_s1ap_id_t ue_id,
  usage_report_response_msg *msg,
  int emm_cause)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  ue_mm_context_t *ue_mm_context = NULL;
  emm_context_t *emm_ctx = NULL;
  int rc = RETURNerror;

  OAILOG_INFO(
    LOG_NAS_EMM,
    "EMM-PROC  - Usage report response (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
    ue_id);
  /*
   * Get the UE context
   */
  ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id(ue_id);
  if (!ue_mm_context) {
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }

  emm_ctx = &ue_mm_context->emm_context;
  nas_emm_usage_report_proc_t *ur_proc = get_nas_common_procedure_usage_report(emm_ctx);

  if (ur_proc) {
    /*
     * Stop timer T3460
     */
    // REQUIREMENT_3GPP_24_301(R10_5_4_3_4__1);

    void *timer_callback_arg = NULL;
    nas_stop_T3460(ue_id, &ur_proc->T3460, timer_callback_arg);

    /*
     * Release retransmission timer parameters
     */

    // verify UE's signature (TODO)
    // uint8_t digest[SHA_DIGEST_LENGTH];
    // SHA1(msg->ue_report, UE_REPORT_SIZE, digest);
    // if(ECDSA_verify(NID_sha1, digest, SHA_DIGEST_LENGTH, (unsigned char *)msg->ue_sig, _get_sig_len(msg->ue_sig), emm_ctx->ue_public_ecdsa) == 1)
    //   return RETURNok;
    // return RETURNerror;

    if (emm_ctx) {
      /*
       * Notify EMM that the authentication procedure successfully completed
       */
      OAILOG_INFO(
      LOG_NAS_EMM,
      "EMM-PROC  - Notify EMM that the usage report procedure successfully "
      "completed\n");
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_COMMON_PROC_CNF;
      emm_sap.u.emm_reg.ue_id = ue_id;
      emm_sap.u.emm_reg.ctx = emm_ctx;
      emm_sap.u.emm_reg.notify = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.common.common_proc = &ur_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
        ur_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      rc = emm_sap_send(&emm_sap);
    }
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  } else {
    OAILOG_ERROR(
      LOG_NAS_EMM,
      "EMM-PROC  - No EMM context exists. Ignoring the Usage Report "
      "Response message\n");
    rc = RETURNerror;
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

static void _usage_report_t3460_handler(void *args)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_context_t *emm_ctx = (emm_context_t *) (args);

  if (!(emm_ctx)) {
    OAILOG_ERROR(LOG_NAS_EMM, "T3460 timer expired No EMM context\n");
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }
  nas_emm_usage_report_proc_t *ur_proc = get_nas_common_procedure_usage_report(emm_ctx); 

  if (ur_proc) {
    /*
     * Increment the retransmission counter
     */
    ur_proc->retransmission_count += 1;
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - T3460 timer expired, retransmission "
      "counter = %d\n",
      ur_proc->retransmission_count);
    if (USAGE_REPORT_COUNTER_MAX > ur_proc->retransmission_count) {
      // REQUIREMENT_3GPP_24_301(R10_5_4_3_7_b__1);
      /*
       * Send security mode command message to the UE
       */
      _usage_report_request(ur_proc);
    } else {
      // REQUIREMENT_3GPP_24_301(R10_5_4_3_7_b__2);
      /*
     * Abort the security mode control and attach procedure
     */
      increment_counter(
        "nas_security_mode_command_timer_expired", 1, NO_LABELS);
      increment_counter(
        "ue_attach",
        1,
        2,
        "result",
        "failure",
        "cause",
        "no_response_for_security_mode_command");
      _usage_report_abort(emm_ctx, (struct nas_base_proc_s *) ur_proc);
      emm_common_cleanup_by_ueid(ur_proc->ue_id);
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ur_proc->ue_id;
      emm_sap_send(&emm_sap);
      increment_counter("ue_attach", 1, 1, "action", "attach_abort");
    }
  }
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

static int _usage_report_abort(
  emm_context_t *emm_ctx,
  struct nas_base_proc_s *base_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  unsigned int ue_id;

  if (emm_ctx && base_proc) {
    nas_emm_usage_report_proc_t *ur_proc = (nas_emm_usage_report_proc_t *) base_proc;
    ue_id = ur_proc->ue_id;
    OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC - Abort usage report\
                    procedure "
      "(ue_id=" MME_UE_S1AP_ID_FMT ")\n",
      ue_id);
    /*
       * Stop timer T3460
       */
    if (ur_proc->T3460.id != NAS_TIMER_INACTIVE_ID) {
      OAILOG_INFO(
        LOG_NAS_EMM,
        "EMM-PROC  - Stop timer T3460 (%ld)\n",
        ur_proc->T3460.id);
      nas_stop_T3460(ue_id, &ur_proc->T3460, NULL);
    }
    /*
   * Release retransmission timer parameters
   * Do it after emm_sap_send
   */
    emm_proc_common_clear_args(ue_id);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

static int _usage_report_request(nas_emm_usage_report_proc_t *const ur_proc)
{
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  ue_mm_context_t *ue_mm_context = NULL;
  emm_context_t *emm_ctx = NULL;
  emm_sap_t emm_sap = {0};
  int rc = RETURNerror;

  if (ur_proc) {
    /*
     * Notify EMM-AS SAP that Usage Report Request message has to be sent
     * to the UE
     */
    // REQUIREMENT_3GPP_24_301(R10_5_4_3_2__14);
    emm_sap.primitive = EMMAS_DATA_REQ;
    // emm_sap.u.emm_as.u.security.puid =
    //   ur_proc->emm_com_proc.emm_proc.base_proc.nas_puid;
    emm_sap.u.emm_as.u.data.guti = NULL;
    emm_sap.u.emm_as.u.data.ue_id = ur_proc->ue_id;
    emm_sap.u.emm_as.u.data.ur_report_id = ur_proc->report_id; 
    emm_sap.u.emm_as.u.data.nas_info = EMM_AS_NAS_DATA_UR_REQ; 

    ue_mm_context = mme_ue_context_exists_mme_ue_s1ap_id(ur_proc->ue_id);
    if (ue_mm_context) {
      emm_ctx = &ue_mm_context->emm_context;
    } else {
      OAILOG_ERROR(
        LOG_NAS_EMM,
        "UE MM Context NULL! for ue_id = (%u)\n",
        ur_proc->ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
    }

    /*
   * Setup EPS NAS security data
   */
    emm_as_set_security_data(
      &emm_sap.u.emm_as.u.data.sctx,
      &emm_ctx->_security,
      false, /* initial attach */
      true /* ciphered */);
    rc = emm_sap_send(&emm_sap);

    if (rc != RETURNerror) {
      // REQUIREMENT_3GPP_24_301(R10_5_4_3_2__1);
      void *timer_callback_args = NULL;
      nas_stop_T3460(ur_proc->ue_id, &ur_proc->T3460, timer_callback_args);
      /*
       * Start T3460 timer
       */
      nas_start_T3460(
        ur_proc->ue_id,
        &ur_proc->T3460,
        ur_proc->emm_com_proc.emm_proc.base_proc.time_out,
        emm_ctx);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

