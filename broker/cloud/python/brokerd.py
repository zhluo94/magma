import logging

from broker.protos import brokerd_pb2, brokerd_pb2_grpc


class BrokerdRpcServicer(brokerd_pb2_grpc.BrokerdServicer):
    """
    Brokerd rpc server.
    """

    def __init__(self):
        logging.info("starting brokerd servicer")

    def add_to_server(self, server):
        brokerd_pb2_grpc.add_BrokerdServicer_to_server(self, server)

    def BrokerAuthenticationInformation(self, request, context):
        # TODO:
        #  Upon receiving BrokerAuthenticationInformationRequest, do:
        #  - verify the signature;
        #  - request AuthenticationInformation to subscriberdb;
        #  - sign the response
        #  - create BrokerAuthenticationInformationAnswer and return
        pass
