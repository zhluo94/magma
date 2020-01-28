"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import ipaddress
import time
import unittest

import s1ap_types
import s1ap_wrapper


class TestAttachDetachTwoPDNsWithTcpTraffic(unittest.TestCase):
    def setUp(self):
        self._s1ap_wrapper = s1ap_wrapper.TestWrapper()

    def tearDown(self):
        self._s1ap_wrapper.cleanup()

    def test_attach_detach_two_pdns_with_tcp_traffic(self):
        """ Attach a single UE, send standalone PDN Connectivity
        Request, generate TCP traffic for each PDN session """

        self._s1ap_wrapper.configUEDevice(1)
        req = self._s1ap_wrapper.ue_req
        ue_id = req.ue_id
        print("************************* Running End to End attach for UE id ", ue_id)
        # Attach
        self._s1ap_wrapper.s1_util.attach(
            ue_id,
            s1ap_types.tfwCmd.UE_END_TO_END_ATTACH_REQUEST,
            s1ap_types.tfwCmd.UE_ATTACH_ACCEPT_IND,
            s1ap_types.ueAttachAccept_t,
        )

        # Wait on EMM Information from MME
        self._s1ap_wrapper._s1_util.receive_emm_info()
        default_apn_ip = self._s1ap_wrapper._s1_util.get_ip(ue_id)

        # Send PDN Connectivity Request
        apn = "ims"
        print(
            "************************* Sending Standalone PDN "
            "CONNECTIVITY REQUEST for UE id ",
            ue_id,
            " APN ",
            apn,
        )
        self._s1ap_wrapper.sendPdnConnectivityReq(ue_id, apn)
        # Receive PDN CONN RSP/Activate default EPS bearer context request
        response = self._s1ap_wrapper.s1_util.get_response()
        self.assertEqual(response.msg_type, s1ap_types.tfwCmd.UE_PDN_CONN_RSP_IND.value)
        pdn_conn_rsp = response.cast(s1ap_types.uePdnConRsp_t)
        ims_addr = pdn_conn_rsp.m.pdnInfo.pAddr.addrInfo
        ims_ip = ipaddress.ip_address(bytes(ims_addr[:4]))

        print(
            "************************* Running UE uplink (TCP) for UE id ",
            req.ue_id,
            " UE IP addr: ",
            default_apn_ip,
            " APN: oai_IPv4",
        )
        with self._s1ap_wrapper._trf_util.generate_traffic_test(
            [default_apn_ip], is_uplink=True, duration=5, is_udp=False
        ) as test:
            test.verify()

        print("Sleeping for 5 seconds...")
        time.sleep(5)
        print(
            "************************* Running UE uplink (TCP) for UE id ",
            req.ue_id,
            " ue ip addr: ",
            ims_ip,
            " APN: ",
            apn,
        )
        with self._s1ap_wrapper._trf_util.generate_traffic_test(
            [ims_ip], is_uplink=True, duration=5, is_udp=False
        ) as test:
            test.verify()

        print("Sleeping for 5 seconds...")
        time.sleep(5)
        # Send PDN Disconnect
        print("******************* Sending PDN Disconnect" " request")
        pdn_disconnect_req = s1ap_types.uepdnDisconnectReq_t()
        pdn_disconnect_req.ue_Id = ue_id
        pdn_disconnect_req.epsBearerId = pdn_conn_rsp.m.pdnInfo.epsBearerId
        self._s1ap_wrapper._s1_util.issue_cmd(
            s1ap_types.tfwCmd.UE_PDN_DISCONNECT_REQ, pdn_disconnect_req
        )

        # Receive UE_DEACTIVATE_BER_REQ
        response = self._s1ap_wrapper.s1_util.get_response()
        self.assertEqual(
            response.msg_type, s1ap_types.tfwCmd.UE_DEACTIVATE_BER_REQ.value
        )

        print("******************* Received deactivate EPS bearer context",
            " request")
        deactv_bearer_req = response.cast(s1ap_types.UeDeActvBearCtxtReq_t)
        self._s1ap_wrapper.sendDeactDedicatedBearerAccept(
            req.ue_id, deactv_bearer_req.bearerId
        )

        print(
            "************************* Running UE detach (switch-off) for ",
            "UE id ",
            ue_id,
        )
        # Now detach the UE
        self._s1ap_wrapper.s1_util.detach(
            ue_id, s1ap_types.ueDetachType_t.UE_SWITCHOFF_DETACH.value, False
        )


if __name__ == "__main__":
    unittest.main()