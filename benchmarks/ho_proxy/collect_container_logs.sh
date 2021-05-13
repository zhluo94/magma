#!/bin/bash

for app_name in iperf ping; do
  for fn in $(docker exec -it uec sh -c "ls -d ${app_name}_log_${TEST_SETUP}_*"); do
    fn=$(echo $fn | tr -d '\r')
    if [ -f "$fn" ]; then
      echo "${fn} exists"
    else
      echo "Copy ${fn} from uec"
      docker cp uec:/${fn} .
    fi
  done
done
