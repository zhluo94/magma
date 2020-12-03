"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import logging
from magma.brokerd.brokerd_servicer import BrokerdRpcServicer
from feg.protos.s6a_proxy_pb2_grpc import S6aProxyStub
from lte.protos.mconfig import mconfigs_pb2
from magma.common.service import MagmaService
from magma.common.service_registry import ServiceRegistry
from lte.protos.subscriberdb_pb2_grpc import SubscriberDBStub

def main():
    """ main() for subscriberdb """
    service = MagmaService('brokerd', mconfigs_pb2.BrokerD())

    # Add all servicers to the server
    subd_chan = ServiceRegistry.get_rpc_channel('subscriberdb',
                                           ServiceRegistry.LOCAL)
    s6a_proxy_stub = S6aProxyStub(subd_chan)
    subscriber_stub = SubscriberDBStub(subd_chan)
    brokerd_servicer = BrokerdRpcServicer(s6a_proxy_stub, subscriber_stub)
    brokerd_servicer.add_to_server(service.rpc_server)

    logging.info('brokerd is running!')

    # Run the service loop
    service.run()

    # Cleanup the service
    service.close()


if __name__ == "__main__":
    main()
