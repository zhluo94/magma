# Handover IP emulation when we detect a cell tower change from QCSuper

# Imports
import subprocess

def call_command(command):
    process = subprocess.run(['sudo docker exec -it uec ' + command], shell=True)

# Get current IP:

# Set IP to 0
call_command('ifconfig eth0 0.0.0.0 netmask 0.0.0.0')

# Set IP to 172.17.0.5
call_command('ifconfig eth0 172.17.0.5 netmask 255.255.0.0 broadcast 0.0.0.0')
call_command('route add default gw 172.17.0.1')