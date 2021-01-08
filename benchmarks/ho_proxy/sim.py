#!/usr/bin/python3

"""
Simulate a sequence of handovers with different handover latencies and measure application performance.
Prerequisite: remote container running at 172.17.0.2 with an iperf server.
"""

import sys
import ho
import app
import time
import numpy as np
from datetime import datetime


def run_iperf():
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
        for i in np.arange(0.0, 0.5, 0.1):
            # Set New IP
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            # fp.write("New IP: " + ip)
            # print("New IP: " + ip)
            # Start iperf steam for 15 seconds
            app.start_iperf(t="20", i="0.1", file=fp)
            # Handover at 5 seconds
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Handover at 10 seconds
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Handover at 15 seconds
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Wait for 5 more seconds for iperf stream to finish
            time.sleep(6)

def run_sip():
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

    with open('sip_output.txt', 'w') as fp:
        for i in np.arange(0.0, 0.2, 0.02):
            # Set New IP
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))

            # *Not working at the moment
            # app.start_sip_client(file=fp)

            # Handover at 5 seconds
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Handover at 10 seconds
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Handover at 15 seconds
            try:
                ip = _ip_base + str(next(_ip_pool))
            except StopIteration:
                _ip_pool = iter(range(5, 128))
                ip = _ip_base + str(next(_ip_pool))
            time.sleep(5)
            t = datetime.now().time()
            print("IP Handover: " + ip + ", Time: " + str(t))
            ho.do(new_ip=ip, lat=i)
            # Wait for 5 more seconds for iperf stream to finish
            time.sleep(6)




if __name__ == "__main__":
    if sys.argv[1] == "iperf":
        run_iperf()
    elif sys.argv[1] == "sip":
        run_sip()
