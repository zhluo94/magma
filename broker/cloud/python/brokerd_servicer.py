import logging
import grpc
from broker.protos import brokerd_pb2, brokerd_pb2_grpc
from feg.protos import s6a_proxy_pb2, s6a_proxy_pb2_grpc


def getBrokerAuthenticationInformationAnswer(subd_answer):
    aia = brokerd_pb2.BrokerAuthenticationInformationAnswer()
    aia.error_code = subd_answer.error_code
    for v in subd_answer.eutran_vectors:
        aia.eutran_vectors.extend([getEUTRANVector(v)])
    logging.info("Auth Msg Received")
    return aia

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

    def __init__(self, s6a_proxy_stub: s6a_proxy_pb2_grpc.S6aProxyStub):
        logging.info("starting brokerd servicer")
        self._s6a_proxy_stub = s6a_proxy_stub

    def add_to_server(self, server):
        brokerd_pb2_grpc.add_BrokerdServicer_to_server(self, server)

    def BrokerAuthenticationInformation(self, request, context):
        # TODO:
        #  Upon receiving BrokerAuthenticationInformationRequest, do:
        #  - verify the signature;
        #  - request AuthenticationInformation to subscriberdb;
        #  - sign the response
        #  - create BrokerAuthenticationInformationAnswer and return
        
        # convert broker request
        print('Authentication through broker')
        subd_request = s6a_proxy_pb2.AuthenticationInformationRequest()
        subd_request.user_name = request.user_name
        subd_request.visited_plmn = request.visited_plmn
        subd_request.num_requested_eutran_vectors = request.num_requested_eutran_vectors
        subd_request.immediate_response_preferred = request.immediate_response_preferred
        subd_request.resync_info = request.resync_info

        # send request
        try:
            subd_answer = self._s6a_proxy_stub.AuthenticationInformation(subd_request)
        except grpc.RpcError:
            logging.error('Unable to generate authentication information for subscriber %s. ',
                          request.user_name)
            context.set_code(grpc.StatusCode.NOT_FOUND)
            context.set_details('Failed to generate authentication info in broker')
            return Void()

        # broker answer
        # aia = brokerd_pb2.BrokerAuthenticationInformationAnswer()
        # aia.error_code = subd_answer.error_code
        # #aia.eutran_vectors = subd_answer.eutran_vectors
        # for v in subd_answer.eutran_vectors:
        #     new_v = brokerd_pb2.BrokerAuthenticationInformationAnswer.EUTRANVector()
        #     new_v.rand = v.rand
        #     new_v.xres = v.xres
        #     new_v.autn = v.autn
        #     new_v.kasme = v.kasme
        #     aia.eutran_vectors.extend([new_v])
        # logging.info("Auth Msg Received")
        # return aia
        return getBrokerAuthenticationInformationAnswer(subd_answer)

    def BrokerUpdateLocation(self, request, context):
        # convert broker request
        print('Update location through broker')
        subd_request = s6a_proxy_pb2.UpdateLocationRequest()
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

        # broker answer
        # aia = brokerd_pb2.BrokerUpdateLocationAnswer()
        # aia.error_code = subd_answer.error_code
        # aia.default_context_id = subd_answer.default_context_id
        # aia.total_ambr.CopyFrom(subd_answer.total_ambr)
        # #aia.total_ambr.max_bandwidth_ul = subd_answer.total_ambr.max_bandwidth_ul
        # #aia.total_ambr.max_bandwidth_dl = subd_answer.total_ambr.max_bandwidth_dl
        # aia.all_apns_included = subd_answer.all_apns_included
        # aia.apn = subd_answer.apn
        # aia.msisdn = subd_answer.msisdn
        # aia.NetworkAccessMode = subd_answer.NetworkAccessMode
        # aia.network_access_mode = subd_answer.network_access_mode
        # logging.info("Update Msg Received")
        # return aia
        return getBrokerUpdateLocationAnswer(subd_answer)

    def BrokerPurgeUE(self, request, context):
        # convert broker request
        print('Purge UE through broker')
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

