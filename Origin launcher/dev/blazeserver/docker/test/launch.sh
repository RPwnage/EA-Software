#!/bin/bash

declare -A baseInstanceIds=( ["configMaster"]=1 ["coreNSMaster"]=2 ["coreMaster"]=3 ["coreSlave"]=12 ["mmSlave"]=32 ["grSlave"]=52 ["searchSlave"]=72 ["gdprAuxSlave"]=92 )

# Get our instance id from our container name
cid=$(cat /proc/self/cgroup | grep "^1:" | grep -oE "/docker/.*")
cName=$(curl http://localhost:4243/containers/${cid#/docker/}/json | jq -r '.Name')
cInfo=${cName#*_}
itype=${cInfo%_*}
iid=$((${cInfo#*_}-1))
inid=$((${iid}+${baseInstanceIds[$itype]}))
iname="${itype}${inid}"
bport=$((${BLAZE_BASE_PORT}+${inid}*20))

dbHostOverride=""
if [ -n "$DBHOST_OVERRIDE" ]; then
  dbHostOverride="-DDBHOST=$DBHOST_OVERRIDE"
fi

logOutput=""
if [ -n "$BLAZE_LOG_TYPE" ] && [ "$BLAZE_LOG_TYPE" == "file" ]; then
  logOutput="--logdir . --logname $iname"
fi

../bin/blazeserver --name $iname --port $bport --id $inid --type $itype $dbHostOverride $logOutput $BLAZE_RUN_ARGS
