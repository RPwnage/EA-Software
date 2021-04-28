#!/bin/bash

#chmod 744 rpc_ratecap.sh

if [ ! -f "rpc_rate.cfg" ]; then
    echo "$0 requires rpc_rate.cfg file."
    exit 1
fi

source rpc_rate.cfg

echo "started snap."

for server in ${!serverCounts[@]};
do
    echo "instance: $server, count: ${serverCounts[$server]}"
done

echo "service: $serviceName"

timestamp=`date -u +%Y%m%d%H%M%S`
rootDir="$serviceName-$timestamp"
mkdir -p $rootDir

timeRange=""

if [ -n "$from" -a -n "$till" ];
then
    echo "qry time range: $from - $till"
    # replace : with %3A
    timeRange=`echo "&from=$from&until=$till" | sed -e s/:/%3A/`
else
    echo "qry time range: last 24h"
fi

echo "created $rootDir."

for server in ${!serverCounts[@]};
do
    declare -i serverCount
    serverCount=${serverCounts[$server]}
    for serverNum in `seq 1 $serverCount`;
    do
        serverName=$server
        outDir="$rootDir/$server"
        mkdir -p $outDir
        if [ $serverCount -gt 1 ];
        then
            serverName="$server$serverNum"
        fi
        echo "getting RPC rate data for $serverName"
        graphiteIn="http://$graphiteServer/render/?target=nonNegativeDerivative($serviceName.$serverName.*.rpc.*.count%2C0)&rawData=true$timeRange"
        graphiteOut="$outDir/$serverName.csv"
        curlCmd="curl -m 30 -s $graphiteIn > $graphiteOut &"
        echo "$curlCmd"
        curl -m 300 -s $graphiteIn > $graphiteOut &
    done
done

echo "wait for all spawned processes."

wait

echo "ending snap."

# echo "archiving results."

# tar czvf $rootDir.tar.gz $rootDir

# rm -rf $rootDir

echo "done."

