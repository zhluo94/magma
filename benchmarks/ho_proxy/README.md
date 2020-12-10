#Handover Proxy

>  [TBD] The handover proxy enables emulating handover over existing cellular infrastrucuture across a pair of client device and server.

### VPN 

> [TBD] @mark

### OvS

This setup leverates docker and [Open vSwitch (OvS)](https://github.com/openvswitch/ovs)

One can set up GRE tunnels connecting two Docker containers with commands:

On each host (client and server):
`ovs-vsctl add-br br-cell`
`sudo ip link add veth0 type veth peer name veth1`
`sudo ovs-vsctl add-port br-cell veth1`
`sudo brctl addif docker0 veth0`
`sudo ip link set veth1 up`
`sudo ip link set veth0 up`

Add tunnel:
`ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST2]`
`ovs-vsctl add-port br-cell gre0 -- set interface gre0 type=gre options:remote_ip=[HOST1]`

Change IP:

> TBD









