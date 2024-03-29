OVS_BR:=br-cell

WG_CONF:=wg0
ifndef WG_REMOTE  # if server: WG_REMOTE=192.168.4.2 make tun
WG_REMOTE:=192.168.4.1
endif

WG_LOCAL:=$(shell cat /etc/wireguard/wg0.conf | grep Address | cut -d' ' -f3)
ifeq ($(WG_LOCAL), $(WG_REMOTE))
$(info wg remote and local ip shouldn't be equal, remote: $(WG_REMOTE))
endif

# dongle config
.PHONY: dongle
dongle:
	$(info yo, do it by hand and README pls)

# pick up and proxy handover signal
.PHONY: mon proxy
mon:
	cd ./QCSuper; sudo ./qcsuper.py --usb-modem /dev/ttyUSB0 --wireshark-live
proxy:
	sudo unbuffer python3 proxy.py | tee proxy_log_${TEST_SETUP}_$(date +"%F-%T")	

# tunnels
.PHONY: wg br gre tun del-tun
wg:
	sudo systemctl start wg-quick@$(WG_CONF)
br:
	sudo ovs-vsctl add-br $(OVS_BR) || true
gre: | br
	sudo ip link add veth0 type veth peer name veth1
	sudo ovs-vsctl add-port $(OVS_BR) veth1
	sudo brctl addif docker0 veth0
	sudo ip link set veth1 up
	sudo ip link set veth0 up
	sudo ovs-vsctl add-port $(OVS_BR) gre0 -- set interface gre0 type=gre options:remote_ip=$(WG_REMOTE)
	sudo ovs-vsctl show

tun: | wg gre
	$(info started tunnels)
del-tun:
	sudo systemctl stop wg-quick@$(WG_CONF) || true
	sudo ovs-vsctl del-br $(OVS_BR) || true
	sudo ip link delete veth0 || true
	sudo ip link delete gre0 || true
	sudo ip link delete gretap0 || true
	sudo ip link delete gre_sys || true
	$(info stopped tunnels)

# vxlan
.PHONY: vxlan
vxlan: | br
	$(info see README)

# other net changes
.PHONY: nat mtu
nat:
	$(info see README)
mtu: 
	ifconfig wg0 mtu 1600 up

# mptcp
.PHONY: mptcp tracebox
mptcp:
	curl http://www.multipath-tcp.org
tracebox:
	sudo tracebox -n -p IP/TCP/MSS/MPCAPABLE/WSCALE www.multipath-tcp.org

# container
.PHONY: con
con:
	sudo docker run --name uec --privileged -itd --mac-address $(CON_MAC) silveryfu/celleval:latest

# dongle
.PHONY: qc
qc:
	sudo ./QCSuper/qcsuper.py --usb-modem /dev/ttyUSB0 --wireshark-live # replace ttyUSB0 if different serial file
