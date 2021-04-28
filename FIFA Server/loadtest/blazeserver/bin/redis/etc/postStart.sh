#!/bin/bash

ipinfo=($(cat /etc/hosts | grep $(hostname)))
nameArr=(${POD_NAME//-/ })
len=${#nameArr[@]}
index=${nameArr[$((len-1))]}

cfgFile="/data/redis_${CLUSTER_NAME}.cfg"
hostname="${ipinfo[0]}:$REDIS_PORT"
updateCfg=1
if [ -f $cfgFile ]; then
  existing=$(cat $cfgFile | grep ^$hostname)
  if [ -n "$existing" ]; then
    updateCfg=0
  fi
fi

if [ "$updateCfg" -ne 0 ]; then
  echo "$hostname" >> $cfgFile
fi

if [ "$index" -eq $((NUM_PODS-1)) ]; then
  allpods=($(cat $cfgFile))
  echo "allpods: ${allpods[@]}"
  redis-cli --cluster create ${allpods[@]} --cluster-replicas $REPLICAS --cluster-yes > /data/redis-${CLUSTER_NAME}-cli.log
fi


