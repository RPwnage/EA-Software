#!/bin/bash

if [ ! -f /app/init/http.port ]; then
  json=$(curl localhost:11021/ports/$POD_NAME/internal1?pool=internal)
  json=${json#\{}
  json=${json%\}}
  jsonArr=(${json//,/ })
  for field in ${jsonArr[@]}; do
    arr=(${field//:/ })
    if [ ${arr[0]} == "\"Host\"" ]; then
      echo "host=${arr[1]}" >> /app/init/http.port
    elif [ ${arr[0]} == "\"PortNumber\"" ]; then
      echo "port=${arr[1]}" >> /app/init/http.port
    fi
  done
  fi

  source /app/init/http.port
  resp=$(curl http://$host:$port/blazecontroller/getDrainStatus)
  inservice=$(echo $resp | grep "<inservice>1</inservice>")
  if [ -n "$inservice" ]; then
  draining=$(echo $resp | grep "<isdraining>0</isdraining>")
  if [ -n "$draining" ]; then
    exit 0
  fi
fi
exit 1