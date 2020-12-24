#!/usr/bin/python3

"""
Emulate handover with container ip change and injected delay
"""

import subprocess
from subprocess import PIPE


def change_con_ip(new_ip, name="uec", ifac="eth0",
                  netmask="255.255.0.0", gw="172.17.0.1",
                  lat=0, msg=""):
    _del = "ifconfig {} 0.0.0.0 netmask 0.0.0.0".format(ifac)
    _add = "ifconfig {} {} netmask {} broadcast 0.0.0.0".format(ifac, new_ip, netmask)
    _gw = "route add default gw {}".format(gw)
    _exec = "sudo docker exec -it {} {} > /dev/null 2>&1"

    subprocess.Popen(";".join([
        _exec.format(name, _del),
        "sleep {}".format(lat),
        _exec.format(name, _add),
        _exec.format(name, _gw),
        "" if msg == "" else "echo {}".format(msg),
    ]), shell=True)

def start_iperf(name="uec", mode="c", ip="172.17.0.2", msg=""):
    _cmd = "iperf -{} {} -i 1 -t 30".format(mode, ip)
    if (mode == "s"):
        _cmd = "iperf -{} -i 1 -t 30".format(mode)
    _exec = "sudo docker exec -it {} {} > /dev/null 2>&1"

    p = subprocess.Popen(";".join([
        _exec.format(name, _cmd),
        "" if msg == "" else "echo {}".format(msg),
    ]), shell=True, stdout=PIPE, stderr=PIPE)
    output = p.stdout.read()
    print(output)


def do(*args, f=change_con_ip, **kwargs):
    f(*args, **kwargs)


if __name__ == "__main__":
    do("172.17.0.128", lat=1)
