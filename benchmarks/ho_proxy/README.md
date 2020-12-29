# Handover Proxy

Handover proxy enables emulating handover over existing cellular infrastrucuture across a pair of client device and server.

### Prerequisites

* Kernel MPTCP-enable
* Open vSwitch
* Docker
* Wireguard (if VPN)

> [TBD] The overlay network setup across cellular WAN consists of a VPN (wireguard) and GRE (or VXLAN) tunnel. 

### VPN 

Wireguard server:
```
[Interface]
PrivateKey = ACBB..
ListenPort = 55107
Address = 192.168.4.1

[Peer]
PublicKey = iOirBGr20tUOtUj9slPG6dwPia1wu3+CnVntqP5ZPAQ=
AllowedIPs = 192.168.4.2/32, 192.168.4.3/32
PersistentKeepalive = 25
```

Wireguard client (UE):

```
[Interface]
Address = 192.168.4.2
PrivateKey = qL0a..
ListenPort = 51820

[Peer]
PublicKey = kjdIWG0i6haYgmnmPkUxAPLSJK9CRGR9b9YjKWphSkk=
AllowedIPs = 192.168.4.1/32
Endpoint = [HOST2]:55107
PersistentKeepalive = 25
```

Save the config as `/etc/wireguard/wg0.conf`. Run`sudo systemctl start wg-quick@wg0`.

Set up GRE tunnels (see below).

### OvS

This setup leverages docker and [Open vSwitch (OvS)](https://github.com/openvswitch/ovs)

One can set up VXLAN (or GRE) tunnels connecting two Docker containers with commands:

On each host (client and server):
```bash
# add bridge br-cell
sudo ovs-vsctl add-br br-cell
# add veth pairs
sudo ip link add veth0 type veth peer name veth1
sudo ovs-vsctl add-port br-cell veth1
# add veth0 to docker default bridge
sudo brctl addif docker0 veth0
# bring online
sudo ip link set veth1 up
sudo ip link set veth0 up
```

Add tunnel, using the host's external IP:
```bash
sudo ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST2]
sudo ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST1]
# GRE: replace type=gre with type=vxlan
```

Start containers on both hosts:

```bash
# host 1
sudo docker run --name uec --privileged -itd --mac-address 00:00:00:00:00:10 ubuntu
# host 2
sudo docker run --name uec --privileged -itd --mac-address 00:00:00:00:00:20 ubuntu
```

Install dependencies:

```bash
sudo docker exec -it [CONTAINER_NAME] /bin/bash
# or use image `silveryfu/celleval:latest`
sudo apt update; apt install curl net-tools iputils-ping iperf git iproute2 -y
# ..experiments
```

Change IP:

```bash
# default to 172.17.0.X, change X -> Y
ifconfig eth0 172.17.0.[Y] netmask 255.255.0.0 broadcast 0.0.0.0
route add default gw 172.17.0.1
# play with interface 
ifconfig eth0 down 
ifconfig eth0 up
# manipulate ip
ifconfig eth0 0.0.0.0 netmask 0.0.0.0
```

Iperf test:

```bash
# on server container, host 1
iperf -s -i 1
# in client container, host 2
iperf -c [container_HOST1] -i 1 -b 1m -t 60
```

Script to change IP of container:

```bash
python3 ho.py
```

NAT hole punching (behind cellular PGW), e.g., make a NAT state for VXLAN traffic:

```
echo -n "hello" | nc -u -w0 -p 4789 [HOST2] 12345
```

Use tcpdump etc. to check the external IP and port (e.g., `-i eth0 port 12345 -nn`), then set up the local iptables described as what follows.

DNAT at HOST2/server:
```
iptables -t nat -A OUTPUT -p udp -d [Remote Tunnel IP] --dport 4789 -j DNAT --to-destination [EXT_IP]:[EXT_PORT]
```

SNAT at HOST2:
```
iptables -t nat -A POSTROUTING -p udp --destination [Ditto] -j SNAT --to-source :12345
```

Note: for all tunneling setup, remember to adjust MTU properly (e.g., 1600). E.g., for wg0:
```
ifconfig wg0 mtu 1600 up
```


