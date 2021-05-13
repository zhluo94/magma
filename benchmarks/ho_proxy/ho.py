#!/usr/bin/python3

"""
Emulate handover with container ip change and injected delay
"""

import subprocess
from random import randrange

d_exec_lat = 150 * 1e-3 # hardcoded for now

def change_con_ip(new_ip, name="uec", ifac="eth0",
                  netmask="255.255.0.0", gw="172.17.0.1",
                  lat=0, msg=""):
    _del = "ifconfig {} 0.0.0.0 netmask 0.0.0.0".format(ifac)
    _sleep = "sleep {}".format(lat)
    _add = "ifconfig {} {} netmask {} broadcast 0.0.0.0".format(ifac, new_ip, netmask)
    _gw = "route add default gw {}".format(gw)
    _msg = "" if msg == "" else "echo {}".format(msg)
    _kill = "false" #"pkill wget" # for wget tcp to react to ip changes
    _exec = "docker exec -it {} /bin/bash -c '{}' > /dev/null 2>&1"

    _cmd = _exec.format(name, ";".join([_del, _sleep, _add, _gw, _kill, _msg]))
    subprocess.Popen(_cmd, shell=True)

   # _sip_ip_cmd = "sleep {}; kill -10 $(pgrep dump_call.exp)".format(d_exec_lat + lat)
   # subprocess.Popen(_sip_ip_cmd, shell=True)

def do(*args, f=change_con_ip, **kwargs):
    f(*args, **kwargs)


if __name__ == "__main__":
    do("172.17.0.{}".format(randrange(3, 256)), f=change_con_ip, lat=256 *1e-3, name="uec_new2")
