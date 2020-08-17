import logging

from broker.protos import brokerd_pb2, brokerd_pb2_grpc
from broker.protos.subscriberdb_pb2_grpc import SubscriberDBStub


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
        subd_answer = self._subscriberdb_stub.AuthenticationInformation(request)
        aia = s6a_proxy_pb2.AuthenticationInformationAnswer()
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
