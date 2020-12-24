#!/usr/bin/python3

"""
Run a sequence of emulation handovers with different handover latencies.
Prerequesite, remote container running at 172.17.0.2 with an iperf server.
"""

import ho
import time
import numpy as np

def run():

    # define IP Pool
    _ip_base = "172.17.0."
    _ip_pool = iter(range(5, 128))

    # Set initial IP
    try:
        ip = _ip_base + str(next(_ip_pool))
    except StopIteration:
        _ip_pool = iter(range(5, 128))
        ip = _ip_base + str(next(_ip_pool))
    ho.do(new_ip=ip, lat=0)

    # Data collection
    with open('run_output.txt', 'w') as fp:
        for i in np.arange(0.0, 0.05, 0.01):
            # Set New IP
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            fp.write("New IP: " + ip)
            print("New IP: " + ip)
            # Start iperf steam for 15 seconds
            ho.start_iperf(t="15", file=fp)
            time.sleep(5)
            # Handover at 5 seconds
            ho.do(new_ip=ip, lat=i)
