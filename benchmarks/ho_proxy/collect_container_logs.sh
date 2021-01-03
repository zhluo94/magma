#!/bin/bash

for app_name in iperf ping; do
  for fn in $(docker exec -it uec ls -d ${app_name}_log_${TEST_SETUP}_*); do
    fn=$(echo $fn | tr -d '\r')
    if [ -f "$fn" ]; then
      echo "${fn} exists"
    else
      docker cp uec:/${fn} .
    fi
  done
done
