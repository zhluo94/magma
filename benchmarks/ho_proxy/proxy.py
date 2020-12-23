#!/usr/bin/python3

"""
Watch handover signals of the pcap stream and update interface ip

Output csv format to stdout.
"""

import pyshark
import os
import time
from struct import unpack
import select
import ho

_pipe = "/tmp/pcap_buffer"


def get_packet(fd: int) -> (bytes, bytes):
    ts, pkt_size = unpack("<II", os.read(fd, 8))
    return ts, os.read(fd, pkt_size)


def loop():
    imc = pyshark.InMemCapture(linktype=228)

    source = os.open(_pipe, os.O_RDONLY | os.O_NONBLOCK)

    poll = select.poll()
    poll.register(source, select.POLLIN)

    _ip_pool = iter(range(128))
    while True:
        if (source, select.POLLIN) in poll.poll(2000):  # 2s
            ts, pkt = get_packet(source)
            # print(msg.decode())
        else:
            continue

        pkt = imc.parse_packet(pkt)

        # print(pkt, dir(pkt.lte_rrc))
        # print("timestamp:", ts)

        # cell id
        cell_id = None
        try:
            cell_id = pkt.lte_rrc.physcellid
        except AttributeError:
            pass

        # handover
        handover = False
        try:
            handover = pkt.lte_rrc.lte_rrc_mobilitycontrolinfo_element == 'mobilityControlInfo'
        except AttributeError:
            pass

        # record
        if handover:
            try:
                ip = next(_ip_pool)
            except StopIteration:
                _ip_pool = iter(range(128))
                ip = next(_ip_pool)

            ho.do(new_ip=ip)

        # TBD which timestamp
        print(time.time(), handover, cell_id)


if __name__ == "__main__":
    loop()
