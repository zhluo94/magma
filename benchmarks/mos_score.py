#!/usr/bin/python3

"""
Calculate MOS score based on call statistics
"""

import sys

def calculate_mos(lat, jit, loss):
    latency = float(lat)
    jitter = float(jit)
    pktloss = float(loss)
    effective_latency = latency + jitter * 2 + 10
    if effective_latency < 160:
        R = 93.2 - (effective_latency / 40)
    else:
        R = 93.2 - ((effective_latency - 120) / 10)

    R = R - (pktloss * 2.5)

    MOS = 1 + (0.035 * R) + (0.000007 * R * (R - 60) * (100 -R))
    
    print(MOS)
    return MOS

if __name__ == "__main__":
    calculate_mos(sys.argv[1], sys.argv[2], sys.argv[3])
