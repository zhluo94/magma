"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import unittest

import s1ap_types

from integ_tests.s1aptests import s1ap_wrapper

from os.path import expanduser

from time import sleep

from integ_tests import sub_config

class TestAttachDetach(unittest.TestCase):

    def setUp(self):
        sub_config.use_orc8r = True
        self._s1ap_wrapper = s1ap_wrapper.TestWrapper()

    def tearDown(self):
        self._s1ap_wrapper.cleanup()
        sub_config.use_orc8r = False

    def test_attach_detach(self):
        """ Basic attach/detach test with a single UE """
        num_ues = 10
        #detach_type = [s1ap_types.ueDetachType_t.UE_NORMAL_DETACH.value,
        #               s1ap_types.ueDetachType_t.UE_SWITCHOFF_DETACH.value]
        #wait_for_s1 = [True, False]
        self._s1ap_wrapper.configUEDevice(num_ues)
        latency_results = []

        for i in range(num_ues):
            req = self._s1ap_wrapper.ue_req
            print("************************* Running End to End attach for ",
                  "UE id ", req.ue_id)
            for _ in range(1):
                # Now actually complete the attach
                _, latency =self._s1ap_wrapper._s1_util.attach(
                    req.ue_id, s1ap_types.tfwCmd.UE_END_TO_END_ATTACH_REQUEST,
                    s1ap_types.tfwCmd.UE_ATTACH_ACCEPT_IND,
                    s1ap_types.ueAttachAccept_t, use_broker=True)

                print('attach latency {} ms'.format(latency))
                # Wait on EMM Information from MME
                self._s1ap_wrapper._s1_util.receive_emm_info()
                print("************************* Running UE detach for UE id ",
                      req.ue_id)
                # Now detach the UE
                if i < num_ues - 1:
                    detach_type = s1ap_types.ueDetachType_t.UE_NORMAL_DETACH.value
                    wait_for_s1 = True
                else:
                    detach_type = s1ap_types.ueDetachType_t.UE_SWITCHOFF_DETACH.value
                    wait_for_s1 = False

                self._s1ap_wrapper.s1_util.detach(
                    req.ue_id, detach_type, wait_for_s1)

                latency_results.append(latency)
            
        with open(expanduser('~/magma/lte/gateway/python/integ_tests/bt_attach_latency.txt'), 'w') as f:
            for l in latency_results:
                f.write(str(round(l, 2)))
                f.write('\n')


if __name__ == "__main__":
    unittest.main()
