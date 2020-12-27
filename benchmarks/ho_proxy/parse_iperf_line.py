#!/usr/bin/python3

"""
Parse an iperf text file output and insert into CSV for analysis.
"""

import sys
import pandas as pd

def txt(filename="", t="15", i="0.01"):
    if (filename):
        with open(filename + ".txt", 'r') as fp:
            line = fp.readline()
            with open(filename + ".csv", 'w') as emtpy_csv:
                pass
            prevline = ""
            cnt = 1
            iperf_cnt = 0
            line_cnt = 0
            collect_data = False
            col = []
            while line:
                if ("------" in line and collect_data):
                    try:
                        csv_input = pd.read_csv(filename + ".csv")
                    except pd.errors.EmptyDataError:
                        fr = pd.DataFrame(col, columns=["HO_Lat: " + str(float(i) * iperf_cnt)])
                        fr.to_csv(filename + ".csv", index=False)
                    else:
                        col_name = "HO_Lat: " + str(iperf_cnt * float(i))
                        csv_input[col_name] = pd.Series(col)
                        csv_input.to_csv(filename + ".csv", index=False)
                    collect_data = False
                    col = []
                if (collect_data):
                    splitline = line.split()
                    val = float(splitline[6])
                    if (val < 10):
                        val *= 1000
                    col.append(str(val))
                if ("[ ID] Interval" in  line):
                    iperf_cnt += 1
                    collect_data = True
                prevline = line
                line = fp.readline()
                cnt += 1

if __name__ == "__main__":
    txt(sys.argv[1])
