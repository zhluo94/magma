# Equipments

## Study the handover in cellular WAN

### Setup 1: USB Dongle + Laptop

USB Dongle: ZTE MF820B 4G LTE USB Modem (GSM Unlocked)
SIM Card: T-Mobile Prepaid SIM Card 30-Day
Laptop: any

Software: 
* [ModemManager](https://www.freedesktop.org/wiki/Software/ModemManager/), 
* [ho_proxy](https://github.com/zhluo94/magma/tree/master/benchmarks/ho_proxy)
* MPTCP enabled kernel

Steps:
* Activate SIM card
* lsusb - confirm the dongle is detected
* mmcli -L - identify modem id of the dongle
* mmcli -m [modem_id] --simple-connect="apn=[APN]" - connect to the cellular provider
* make sure the dongle is in "connected" mode (mmcli -m [modem id]) and ping -I [iface] google.com
* check the bearer: mmcli -b [bearer id]; identify the interface name, ip, gateway ip..
* `ifconfig [iface] [ip]; route add default gw [gateway ip] [iface]`
* (note: configure the DNS to 8.8.8.8, e.g., via resolvconf)
* cd ho_proxy; make qc - start qcsuper

### Setup 2: Phone (HotSpot, ADB) + Laptop

SIM Card: ditto
Phone: Nexus 6P (rooted)

> TBD

### Setup 3: Phone (ADB) + Laptop
> TBD
