#!/bin/bash

#chmod 744 rpc_logratecap.sh

if [ ! -f "rpc_lograte.cfg" ]; then
    echo "$0 requires rpc_lograte.cfg file."
    exit 1
fi

source rpc_lograte.cfg

echo "started snap."

timestamp=`date -u +%Y%m%d%H%M%S`
rootDir="binlogs-$timestamp"
mkdir -p $rootDir

echo "created $rootDir."

for h in ${!hosts[@]};
do
    host=${hosts[$h]}
    echo "Compressing binary controller logs on $host..."
    binLogTarball=$host.binlogs.tar.gz
    ssh $host "cd $binlogdir; tar czvf ~/$binLogTarball *blazecontroller_binary.log" &
done

echo "waiting for compress jobs..."
wait

for h in ${!hosts[@]};
do
    host=${hosts[$h]}
    binLogTarball=$host.binlogs.tar.gz
    echo "Downloading tarball from host $host..."
    scp $host:$binLogTarball $rootDir/$binLogTarball &
done

echo "waiting for download jobs..."
wait


for h in ${!hosts[@]};
do
    host=${hosts[$h]}
    binLogTarball=$host.binlogs.tar.gz
    echo "Deleting remote tarball on $host..."
    ssh $host "rm -f ~/$binLogTarball" &
done

echo "waiting for all delete jobs..."
wait

echo "ending snap."

echo "done."

