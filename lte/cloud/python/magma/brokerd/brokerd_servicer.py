import logging
import grpc
from broker.protos import brokerd_pb2, brokerd_pb2_grpc
from feg.protos import s6a_proxy_pb2, s6a_proxy_pb2_grpc
from M2Crypto import EC, RSA
from hashlib import sha1
from random import getrandbits
import time
import os
from lte.protos.subscriberdb_pb2_grpc import SubscriberDBStub
from lte.protos.subscriberdb_pb2 import (
    LTESubscription,
    SubscriberData,
    SubscriberState,
    SubscriberID,
    SubscriberUpdate,
)


def getEUTRANVector(v):
    new_v = brokerd_pb2.BrokerAuthenticationInformationAnswer.EUTRANVector()
    new_v.rand = v.rand
    new_v.xres = v.xres
    new_v.autn = v.autn
    new_v.kasme = v.kasme
    return new_v

def getBrokerUpdateLocationAnswer(subd_answer): 
    aia = brokerd_pb2.BrokerUpdateLocationAnswer()
    aia.error_code = subd_answer.error_code
    aia.default_context_id = subd_answer.default_context_id
    aia.total_ambr.CopyFrom(getAggregatedMaximumBitrate(subd_answer.total_ambr))
    aia.all_apns_included = subd_answer.all_apns_included
    for v in subd_answer.apn:
        aia.apn.extend([getAPNConfiguration(v)])
    aia.msisdn = subd_answer.msisdn
    aia.network_access_mode = subd_answer.network_access_mode
    logging.info("Update Msg Received")
    return aia 

def getAggregatedMaximumBitrate(v):
    new_v = brokerd_pb2.BrokerUpdateLocationAnswer.AggregatedMaximumBitrate()
    new_v.max_bandwidth_ul = v.max_bandwidth_ul
    new_v.max_bandwidth_dl = v.max_bandwidth_dl
    return new_v

def getAPNConfiguration(v):
    new_v = brokerd_pb2.BrokerUpdateLocationAnswer.APNConfiguration()
    new_v.context_id = v.context_id
    new_v.service_selection = v.service_selection
    new_v.qos_profile.CopyFrom(getQoSProfile(v.qos_profile))
    new_v.ambr.CopyFrom(getAggregatedMaximumBitrate(v.ambr))
    new_v.pdn = v.pdn
    return new_v

def getQoSProfile(v):
    new_v = brokerd_pb2.BrokerUpdateLocationAnswer.APNConfiguration.QoSProfile()
    new_v.class_id = v.class_id
    new_v.priority_level = v.priority_level
    new_v.preemption_capability = v.preemption_capability
    new_v.preemption_vulnerability = v.preemption_vulnerability
    return new_v

class BrokerdRpcServicer(brokerd_pb2_grpc.BrokerdServicer):
    """
    Brokerd rpc server.
    """

    def __init__(self, s6a_proxy_stub: s6a_proxy_pb2_grpc.S6aProxyStub, subscriber_stub: SubscriberDBStub):
        logging.info("starting brokerd servicer")
        self._s6a_proxy_stub = s6a_proxy_stub
        self._subscriber_stub = subscriber_stub
        key_dir = '/var/opt/magma/key_files'
        self.br_rsa_pri_key =  RSA.load_key(os.path.join(key_dir, 'br_rsa_pri.pem'))
        self.br_ecdsa_pri_key =  EC.load_key(os.path.join(key_dir, 'br_ec_pri.pem'))
        self.ue_rsa_pub_key = RSA.load_pub_key(os.path.join(key_dir, 'ue_rsa_pub.pem'))
        self.ue_ecdsa_pub_key = EC.load_pub_key(os.path.join(key_dir, 'ue_ec_pub.pem'))
        self.ut_rsa_pub_key = RSA.load_pub_key(os.path.join(key_dir, 'ut_rsa_pub.pem'))
        self.ut_ecdsa_pub_key = EC.load_pub_key(os.path.join(key_dir, 'ut_ec_pub.pem'))
        self.br_id = 0
        logging.info("done loading broker keys")

    def add_to_server(self, server):
        brokerd_pb2_grpc.add_BrokerdServicer_to_server(self, server)

    def getBrokerAuthenticationInformationAnswer(self, ue_id, ut_id, nonce, sub_data):
        aia = brokerd_pb2.BrokerAuthenticationInformationAnswer()
        aia.error_code = s6a_proxy_pb2.SUCCESS
        aia.br_auth_vectors.extend([self.getBrokerAuthVector(ue_id, ut_id, nonce, sub_data)])
        logging.info("Auth Msg Received")
        return aia

    def getBrokerAuthVector(self, ue_id, ut_id, nonce, sub_data):
        session_key_size = 32
        ss_ut_br = bytearray(getrandbits(8) for _ in range(session_key_size))
        ss_ue_br = bytearray(getrandbits(8) for _ in range(session_key_size))
        ss_ue_ut = bytearray(getrandbits(8) for _ in range(session_key_size))
        br_ue_plain_text = bytearray(b'')
        br_ue_plain_text.append(self.br_id)
        br_ue_plain_text.append(ut_id)
        br_ue_plain_text.extend(ss_ue_ut)
        br_ue_plain_text.extend(ss_ue_br)
        br_ue_plain_text.extend(nonce)

        br_ut_plain_text = bytearray(b'')
        br_ut_plain_text.append(self.br_id)
        br_ut_plain_text.extend(ss_ue_ut)
        br_ut_plain_text.extend(ss_ut_br)
        br_ut_plain_text.extend(sub_data.SerializeToString()) # sub_data info
        v = brokerd_pb2.BrokerAuthenticationInformationAnswer.BrokerAuthVector()
        v.br_ut_token = self.ut_rsa_pub_key.public_encrypt(br_ut_plain_text, RSA.pkcs1_padding)
        v.br_ut_token_br_sig = self.br_ecdsa_pri_key.sign_dsa_asn1(sha1(v.br_ut_token).digest())
        v.br_ue_token = self.ue_rsa_pub_key.public_encrypt(br_ue_plain_text, RSA.pkcs1_padding)
        v.br_ue_token_br_sig = self.br_ecdsa_pri_key.sign_dsa_asn1(sha1(v.br_ue_token).digest())
        return v

    def BrokerAuthenticationInformation(self, request, context):
        #  Upon receiving BrokerAuthenticationInformationRequest, do:
        #  - verify the signature;
        #  - request AuthenticationInformation to subscriberdb;
        #  - sign the response
        #  - create BrokerAuthenticationInformationAnswer and return
        start = time.time()

        myhash = sha1()
        ue_br_token = request.ue_br_token
        ue_br_token_ue_sig = request.ue_br_token_ue_sig[:request.ue_br_token_ue_sig[1] + 2]
        ue_br_token_ut_sig = request.ue_br_token_ut_sig[:request.ue_br_token_ut_sig[1] + 2]
        # verify token
        plain_token = self.br_rsa_pri_key.private_decrypt(request.ue_br_token, RSA.pkcs1_padding)
        ue_id = plain_token[0]
        ut_id = plain_token[1]
        nonce = plain_token[2:]
        # verify ue signature
        myhash.update(ue_br_token)
        if not self.ue_ecdsa_pub_key.verify_dsa_asn1(myhash.digest(), ue_br_token_ue_sig):
            logging.error('Unable to verify UE signature')
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to verify UE signature')
            return Void()
        else:
            logging.info('Done verifying UE signature')
        # verify ut signature
        myhash.update(ue_br_token_ue_sig)
        if not self.ut_ecdsa_pub_key.verify_dsa_asn1(myhash.digest(), ue_br_token_ut_sig):
            logging.error('Unable to verify UT signature')
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to verify UT signature')
            return Void()
        else:
            logging.info('Done verifying UT signature')
        
        # # update location 
        # ulr = s6a_proxy_pb2.UpdateLocationRequest()
        # ulr.user_name = request.user_name
        # ulr.visited_plmn = request.visited_plmn
        # # ignore skip_subscriber_data and initial_attach for now
        # # send request
        # try:
        #     ula = self._s6a_proxy_stub.UpdateLocation(ulr)
        # except grpc.RpcError:
        #     logging.error('Unable to update location for subscriber %s. ',
        #                   request.user_name)
        #     context.set_code(grpc.StatusCode.NOT_FOUND)
        #     context.set_details('Failed to update location in broker')
        #     return Void()

        # bt_aia = self.getBrokerAuthenticationInformationAnswer(ue_id, ut_id, nonce, ula)

        # get subscriber data
        try:
            sub_data = self._subscriber_stub.GetSubscriberData(SubscriberID(id=request.user_name))
        except grpc.RpcError:
            logging.error('Unable to get subscriber data for subscriber %s. ',
                          request.user_name)
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to get subscriber data in broker')
            return Void()

        bt_aia = self.getBrokerAuthenticationInformationAnswer(ue_id, ut_id, nonce, sub_data)
        
        end = time.time()
        logging.error('BT authentication servicer takes: {} ms'.format((end - start)*1e3))
        return bt_aia

    def BrokerUpdateLocation(self, request, context):
        # convert broker request
        subd_request.user_name = request.user_name
        subd_request.visited_plmn = request.visited_plmn
        subd_request.skip_subscriber_data = request.skip_subscriber_data
        subd_request.initial_attach = request.initial_attach

        # send request
        try:
            subd_answer = self._s6a_proxy_stub.UpdateLocation(subd_request)
        except grpc.RpcError:
            logging.error('Unable to update location for subscriber %s. ',
                          request.user_name)
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to update location in broker')
            return Void()
        return getBrokerUpdateLocationAnswer(subd_answer)

    def BrokerPurgeUE(self, request, context):
        # convert broker request
        subd_request = s6a_proxy_pb2.PurgeUERequest()
        subd_request.user_name = request.user_name

        # send request
        try:
            subd_answer = self._s6a_proxy_stub.PurgeUE(subd_request)
        except grpc.RpcError:
            logging.error('Unable to purge UE for subscriber %s. ',
                          request.user_name)
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to purge UE in broker')
            return Void()

        # broker answer
        aia = brokerd_pb2.BrokerPurgeUEAnswer()
        aia.error_code = subd_answer.error_code
        logging.info("Purge Msg Received")
        return aia

