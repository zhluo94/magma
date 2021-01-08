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
    if filename:
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
                if ("------" in line or "- - - - " in line) and collect_data:
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
                if collect_data:
                    splitline = line.split()
                    val = float(splitline[6])
                    if val < 10:
                        val *= 1000
                    col.append(str(val))
                if "[ ID] Interval" in line:
                    iperf_cnt += 1
                    collect_data = True
                prevline = line
                line = fp.readline()
                cnt += 1
            # col_name = "HO_Lat: " + str(iperf_cnt * float(i))
            # csv_input[col_name] = pd.Series(col)
            # csv_input.to_csv(filename + ".csv", index=False)

def start_sip(name="uec", mode="c", ip="172.17.0.2", file=None):
    _cmd = "pjsua sip:{}:5060 --local-port 5061 --null-audio --no-tcp".format(ip)
    if mode == "s":
        _cmd = "pjsua --play-file /mnt/audio/sound.wav --auto-answer 200 --auto-play --auto-loop --null-audio --no-tcp"
    _exec = "sudo docker exec -it {} {}"

    if file:
        p = subprocess.Popen(";".join([
            _exec.format(name, _cmd)]),
            shell=True, stdout=file, stderr=file, stdin=PIPE)
        time.sleep(5)
        p.stdin.write('h')
        time.sleep(5)
        p.stdin.write('q')
    else:
        p = subprocess.Popen(";".join([
            _exec.format(name, _cmd)]),
            shell=True, stdout=PIPE, stderr=PIPE, stdin=PIPE)
        time.sleep(5)
        p.stdin.write('h')
        time.sleep(5)
        p.stdin.write('q')
        output = p.stdout.read()
        print(str(output, 'utf-8'))


if __name__ == "__main__":
    parse_iperf(sys.argv[1])
