#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Illegal number of arguments"
    exit 2
fi 

if [ $1 -eq 0 ]; then
    echo "MPTCP"
    sysctl -w net.mptcp.mptcp_enabled=1
    sysctl -w net.mptcp.mptcp_path_manager=fullmesh
    export TEST_SETUP=mptcp
elif [ $1 -eq 1 ]; then
    echo "TCP w/o IP change"
    sysctl -w net.mptcp.mptcp_enabled=0
    export TEST_SETUP=tcpwo
elif [ $1 -eq 2 ]; then
    echo "TCP w IP change"
    sysctl -w net.mptcp.mptcp_enabled=0
    export TEST_SETUP=tcp
fi
