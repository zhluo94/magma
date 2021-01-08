#!/bin/bash

docker exec -it uec pjsua sip:172.17.0.2:5060 --local-port 5061 --null-audio --no-tcp
