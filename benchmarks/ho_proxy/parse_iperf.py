#!/usr/bin/python3

"""
Parse an iperf text file output and insert into CSV for analysis.
"""

import pandas as pd

def txt(filename="", t="15", i="0.1"):
    input_txt = pd.read_csv('mptcp_1.txt', delim_whitespace=True)
    input_txt.to_csv(filename + ".csv", index=False)
