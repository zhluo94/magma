#!/usr/bin/python3

"""
Benchmark applications.
"""

import sys
import pandas as pd
import subprocess
import time
from subprocess import PIPE


def start_iperf(name="uec", mode="c", ip="172.17.0.2", t="10", i="1", msg="", file=None):
    _cmd = "iperf3 -R -{} {} -i {} -t {}".format(mode, ip, i, t)
    if mode == "s":
        _cmd = "iperf3 -{} -i {} -t {}".format(mode, i, t)
    _exec = "sudo docker exec -it {} {}"

    if file:
        p = subprocess.Popen(";".join([
            _exec.format(name, _cmd),
            "" if msg == "" else "echo {}".format(msg),
        ]), shell=True, stdout=file, stderr=file)
    else:
        p = subprocess.Popen(";".join([
            _exec.format(name, _cmd),
            "" if msg == "" else "echo {}".format(msg),
        ]), shell=True, stdout=PIPE, stderr=PIPE)
        output = p.stdout.read()
        print(str(output, 'utf-8'))


def parse_iperf(filename="", t="15", i="0.01"):
    """
    Parse an iperf text file output and insert into CSV.
    """
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
                if (("------" in line or "- - - - " in line) and collect_data):
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
                if ("[ ID] Interval" in  line and ("------" not in prevline and "- - - - " not in prevline)):
                    iperf_cnt += 1
                    collect_data = True
                prevline = line
                line = fp.readline()
                cnt += 1
            # col_name = "HO_Lat: " + str(iperf_cnt * float(i))
            # csv_input[col_name] = pd.Series(col)
            # csv_input.to_csv(filename + ".csv", index=False)

def parse_iperf_sensitivity(filename="", t="15", i="0.01"):
    """
    Parse an iperf text file output and insert into CSV.
    """
    if (filename):
        with open(filename + ".txt", 'r') as fp:
            line = fp.readline()
            with open(filename + "_sensitivity" + ".csv", 'w') as emtpy_csv:
                pass
            prevline = ""
            cnt = 1
            iperf_cnt = 0
            line_cnt = 0
            collect_data = False
            col = []
            while line:
                if (line == "\n" and collect_data):
                    try:
                        csv_input = pd.read_csv(filename + "_sensitivity" + ".csv")
                    except pd.errors.EmptyDataError:
                        fr = pd.DataFrame(col, columns=["HO_Lat: " + str(float(i) * iperf_cnt)])
                        fr.to_csv(filename + "_sensitivity" + ".csv", index=False)
                    else:
                        col_name = "HO_Lat: " + str(iperf_cnt * float(i))
                        csv_input[col_name] = pd.Series(col)
                        csv_input.to_csv(filename + "_sensitivity" + ".csv", index=False)
                    collect_data = False
                    col = []
                if (collect_data):
                    splitline = line.split()
                    val = float(splitline[6])
                    if (val < 10):
                        val *= 1000
                    col.append(str(val))
                if ("[ ID] Interval" in  line and ("------" in prevline or "- - - - " in prevline)):
                    iperf_cnt += 1
                    collect_data = True
                prevline = line
                line = fp.readline()
                cnt += 1
            # col_name = "HO_Lat: " + str(iperf_cnt * float(i))
            # csv_input[col_name] = pd.Series(col)
            # csv_input.to_csv(filename + "_sensitivity" + ".csv", index=False)

def start_sip_client(file=None):
    _exec = "./dump_call.exp"

    if file:
        p = subprocess.Popen([_exec], shell=True, stdout=file, stderr=file)
    else:
        p = subprocess.Popen([_exec], shell=True, stdout=PIPE, stderr=PIPE)
        output = p.stdout.read()
        print(str(output, 'utf-8'))


if __name__ == "__main__":
    if "iperf_sensitivity" in sys.argv[1]:
        parse_iperf_sensitivity(sys.argv[2])
    else:
        parse_iperf(sys.argv[2])
