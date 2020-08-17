import logging

from broker.protos import brokerd_pb2, brokerd_pb2_grpc
from broker.protos.subscriberdb_pb2_grpc import SubscriberDBStub
from feg.protos import s6a_proxy_pb2, s6a_proxy_pb2_grpc


class BrokerdRpcServicer(brokerd_pb2_grpc.BrokerdServicer):
    """
    Brokerd rpc server.
    """

    def __init__(self, subscriberdb_stub: SubscriberDBStub):
        logging.info("starting brokerd servicer")
        self._subscriberdb_stub = subscriberdb_stub


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
        subd_request = s6a_proxy_pb2.AuthenticationInformationRequest()
        subd_request.user_name = request.user_name
        subd_request.visited_plmn = request.visited_plmn
        subd_request.num_requested_eutran_vectors = request.num_requested_eutran_vectors
        subd_request.immediate_response_preferred = request.immediate_response_preferred
        subd_request.resync_info = request.resync_info

        # send request
        subd_answer = self._subscriberdb_stub.AuthenticationInformation(subd_request)

        # broker answer
        aia = brokerd_pb2.BrokerAuthenticationInformationAnswer()
        aia.error_code = subd_answer.error_code
        aia.eutran_vectors = subd_answer.eutran_vectors
        logging.info("Auth Msg Received")
        return aia

    except CryptoError as e:
        logging.error("Auth error for %s: %s", imsi, e)
        metrics.S6A_AUTH_FAILURE_TOTAL.labels(
            code=metrics.DIAMETER_AUTHENTICATION_REJECTED).inc()
        aia.error_code = metrics.DIAMETER_AUTHENTICATION_REJECTED
        return aia

    except SubscriberNotFoundError as e:
        logging.warning("Subscriber not found: %s", e)
        metrics.S6A_AUTH_FAILURE_TOTAL.labels(
            code=metrics.DIAMETER_ERROR_USER_UNKNOWN).inc()
        aia.error_code = metrics.DIAMETER_ERROR_USER_UNKNOWN
        return aia

    def UpdateLocation(self, request, context):
        # convert broker request
        subd_request = s6a_proxy_pb2.UpdateLocationRequest()
        subd_request.user_name = request.user_name
        subd_request.visited_plmn = request.visited_plmn
        subd_request.skip_subscriber_data = request.skip_subscriber_data
        subd_request.initial_attach = request.initial_attach

        # send request
        subd_answer = self._subscriberdb_stub.UpdateLocation(subd_request)

        # broker answer
        aia = brokerd_pb2.BrokerUpdateLocationAnswer()
        aia.error_code = subd_answer.error_code
        aia.default_context_id = subd_answer.default_context_id
        aia.total_ambr = subd_answer.total_ambr
        aia.all_apns_included = subd_answer.all_apns_included
        aia.apn = subd_answer.apn
        aia.msisdn = subd_answer.msisdn
        aia.NetworkAccessMode = subd_answer.NetworkAccessMode
        aia.network_access_mode = subd_answer.network_access_mode
        logging.info("Update Msg Received")
        return aia
    
    def BrokerPurgeUE(self, request, context):
        # convert broker request
        subd_request = s6a_proxy_pb2.PurgeUERequest()
        subd_request.user_name = request.user_name

        # send request
        subd_answer = self._subscriberdb_stub.PurgeUE(subd_request)

        # broker answer
        aia = brokerd_pb2.BrokerPurgeUEAnswer()
        aia.error_code = subd_answer.error_code
        logging.info("Purge Msg Received")
        return aia

