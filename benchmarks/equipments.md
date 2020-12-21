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
* lsusb
* mmcli -L: 
* mmcli -m [modem_id] --simple-connect="apn=[APN]"
* cd ho_proxy; make start

### Setup 2: Phone (HotSpot, ADB) + Laptop

SIM Card: ditto
Phone: Nexus 6P (rooted)

> TBD

### Setup 3: Phone (ADB) + Laptop
> TBD