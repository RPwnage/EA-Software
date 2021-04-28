#!/bin/bash

#chmod 744 rpc_logrategen.sh

rootDir="$1"

rpcRateScale=""

if [ ! -d "$rootDir" ]; then
    echo "$0 requires directory @param."
    exit 1
fi

if [ ! -f "rpc_lograte.cfg" ]; then
    echo "$0 requires rpc_lograte.cfg file."
    exit 1
fi

if [ ! -z "$2" ]; then
    echo "scaling output by: $2."
    rpcRateScale="-rpcRateScale $2"
fi

source rpc_lograte.cfg

echo "started gen."

echo "extracting binary logs from tarballs..."

pushd $rootDir

for i in *binlogs.tar.gz;
do
    tar -xzvf $i;
done

popd

echo "extraction completed."

# disable pathname wildcard expansion (needed for params to h2logreader)
set -f
 
for i in ${!instances[@]};
do
    instance=${instances[$i]}
    foundFiles=`find $rootDir -name *_$instance*_binary.log | wc -l`
    if [ $foundFiles -gt 0 ]
    then
        echo "generating summary for instance type: $instance..."
        cmd="$h2logreader -f $rootDir/*_$instance*_binary.log -rpcRateSummary $rpcRateScale > $rootDir/$instance.csv"
        echo $cmd
        eval $cmd &
        echo "summary generated."
    else
        echo "no logs for $instance, skipping..."
    fi
done

# re-enable pathname wildcard expansion
set +f

echo "wait for all spawned processes."
wait

echo "deleting extracted binary logs."

# remove all extacted binary logs
rm -f $rootDir/*binary.log

cd $rootDir

csvTar="`echo $rootDir | sed s/binlogs/rpcsummary/g`.tar.gz"

echo "archiving *.csv files > $csvTar."

tar czvf $csvTar *.csv

echo "cleaning *.csv files."

rm -f *.csv

echo "ended gen (summary output: $rootDir)."

echo "done."

