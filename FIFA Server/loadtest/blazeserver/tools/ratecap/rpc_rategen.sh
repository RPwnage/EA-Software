#!/bin/bash

#chmod 744 rpc_rategen.sh

rootDir="$1"

if [ ! -d "$rootDir" ]; then
    echo "$0 requires directory @param."
    exit 1
fi

if [ ! -f "rpc_rate.cfg" ]; then
    echo "$0 requires rpc_rate.cfg file."
    exit 1
fi

source rpc_rate.cfg

echo "started gen."

for server in ${!serverCounts[@]};
do
    echo "instance: $server, count: ${serverCounts[$server]}"
done

echo "process dir $rootDir."

for server in ${!serverCounts[@]};
do
    if [ -d "$rootDir/$server" ]; then
        cat $rootDir/$server/$server*.csv | awk -f rpc_rates.awk > $rootDir/$server-summary.csv &
    fi
done

echo "wait for all spawned processes."

wait

echo "ending gen."

echo "done."

