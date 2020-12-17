# Handover Proxy

>  [TBD] The handover proxy enables emulating handover over existing cellular infrastrucuture across a pair of client device and server.

### Prerequisites

* Kernel MPTCP-enable
* Open vSwitch
* Docker

> [TBD] ..

### VPN 

> [TBD] @mark

### OvS

This setup leverates docker and [Open vSwitch (OvS)](https://github.com/openvswitch/ovs)

One can set up GRE/VXLAN tunnels connecting two Docker containers with commands:

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
sudo ovs-vsctl add-port br-cell vx0 -- set interface vx0 type=vxlan options:remote_ip=[HOST2]
sudo ovs-vsctl add-port br-cell vx0 -- set interface vx0 type=vxlan options:remote_ip=[HOST1]
```

Start containers on both hosts:

```bash
# host 1
sudo docker run --name uec --privileged -itd --mac-address 00:00:00:00:00:11 ubuntu
# host 2
sudo docker run --name uec --privileged -itd --mac-address 00:00:00:00:00:12 ubuntu
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

Script to change IP of container, emulating handover:
```bash
python3 ovs_handover.py
```











