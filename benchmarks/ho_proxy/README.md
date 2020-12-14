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

One can set up GRE tunnels connecting two Docker containers with commands:

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

Add tunnel, usie host1/2 physical adapter IP:
```bash
sudo ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST2]
sudo ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST1]
```

Start containers (client and server, using containers name uec):

```bash
sudo docker run --name uec --privileged -itd ubuntu
```

Install dependencies:

```bash
sudo docker exec -it [CONTAINER_NAME] /bin/bash
# Or use image `silveryfu/celleval:latest`
sudo apt update; apt install curl net-tools iputils-ping iperf git -y
# ..experiments
```

Change IP:

```bash
# play with interface 
sudo ifconfig eth0 down 
sudo ifconfig eth0 up
# manipulate ip
sudo ifconfig eth0 0.0.0.0 netmask 0.0.0.0
# default to 172.17.0.X, change X -> Y
sudo ifconfig eth0 172.17.0.[Y] netmask 255.255.0.0 broadcast 0.0.0.0
sudo route add default gw 172.17.0.1
```

Iperf test:

```bash
# on server container, HOST1
iperf -s -i 1
# in client container, HOST2
iperf -c [container_HOST1] -i 1 -t 60
```

Script to change IP of container, emulating handover:
```bash
python3 ovs_handover.py
```











