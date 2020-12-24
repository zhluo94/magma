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
    _ip_pool = iter(range(5, 128))

    # Set initial IP
    try:
        ip = next(_ip_pool)
    except StopIteration:
        _ip_pool = iter(range(5, 128))
        ip = next(_ip_pool)
    ho.do(new_ip=ip, lat=0)

    # Data collection
    with open('run_output.txt', 'w') as fp:
        for i in np.arange(0.0, 1.0, 0.01):
            # Set New IP
            try:
                ip = next(_ip_pool)
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = next(_ip_pool)
            # Start iperf steam for 15 seconds
            ho.start_iperf(t="15", file=fp)
            time.sleep(5)
            # Handover at 5 seconds
            ho.do(new_ip=ip, lat=i)
