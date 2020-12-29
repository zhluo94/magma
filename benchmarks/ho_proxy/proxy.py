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
import app

_pipe = "/tmp/pcap_buffer"


def get_packet(fd: int) -> (bytes, bytes):
    ts, pkt_size = unpack("<II", os.read(fd, 8))
    return ts, os.read(fd, pkt_size)


def loop():
    imc = pyshark.InMemCapture(linktype=228)

    source = os.open(_pipe, os.O_RDONLY | os.O_NONBLOCK)

    poll = select.poll()
    poll.register(source, select.POLLIN)

    _ip_base = "172.17.0."
    _ip_pool = iter(range(5, 128))
    handover_start = False
    handover_complete = False
    while True:
        if (source, select.POLLIN) in poll.poll(2000):  # 2s
            ts, pkt = get_packet(source)
        else:
            continue

        pkt = imc.parse_packet(pkt)

        # print("timestamp:", ts, pkt, dir(pkt.lte_rrc))

        # cell id
        cell_id = None
        try:
            cell_id = pkt.lte_rrc.physcellid
        except AttributeError:
            pass

        # Check handover completes
        if handover_start:
            handover_complete = False
            try:
                handover_complete = pkt.lte_rrc.lte_rrc_rrcconnectionreconfigurationcomplete_element == 'rrcConnectionReconfigurationComplete'
            except AttributeError:
                pass

            # Record
            if (handover_complete):
                try:
                    ip = _ip_base + str(next(_ip_pool))
                except StopIteration:
                    _ip_pool = iter(range(5, 128))
                    ip = _ip_base + str(next(_ip_pool))

                app.do(new_ip=ip, lat=0.02)
            handover_start = False

        # handover start
        try:
            handover_start = pkt.lte_rrc.lte_rrc_mobilitycontrolinfo_element == 'mobilityControlInfo'
        except AttributeError:
            pass

        # TBD which timestamp
        print(time.time(), handover_complete, cell_id)


if __name__ == "__main__":
    loop()
