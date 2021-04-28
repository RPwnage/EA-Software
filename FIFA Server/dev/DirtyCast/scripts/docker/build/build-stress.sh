#!/bin/bash
# bash script automating build phase.

function setClientExport()
{
    local newVal=$1
    local file=$2
    local lineNo=$(grep -n " stressLogin" -B5 ${file} | grep "client_export" | cut -f1 -d-)
    local currVal=$(sed "${lineNo}q;d" ${file} | cut -d= -f2)
    sed -i "${lineNo}s|${currVal}| ${newVal},|" ${file}
}


cd /tmp/EAGS/gos-stream/ml/DirtyCast

if [[ ( -z "$PKG_USER" ) || ( -z "$PKG_PASSWORD" ) ]]; then
    mono ../Framework/version/bin/eapm.exe credstore -use-package-server
else
    mono ../Framework/version/bin/eapm.exe credstore -user:$PKG_USER -password:$PKG_PASSWORD -use-package-server
fi

# Find the RPC file
rpcFile='gen/component/authentication/gen/authenticationcomponent.rpc'
if [ -f "/tmp/EAGS/gos-stream/ml/BlazeSDK/${rpcFile}" ]; then
    rpcFile="/tmp/EAGS/gos-stream/ml/BlazeSDK/${rpcFile}"
else
    # Use nant build to download the packages
    mono ../Framework/version/bin/eapm.exe install BlazeSDK -masterconfigfile:unix-masterconfig.xml

    cd /tmp/EAGS/gos-stream/packages/BlazeSDK
    cd $(ls) # package version folder
    rpcFile="$(pwd)/${rpcFile}"
    if [ ! -f ${rpcFile} ]; then
        echo "RPC file not found"
        exit 1
    fi
    cd /tmp/EAGS/gos-stream/ml/DirtyCast
fi

# Enable Stress Login
setClientExport "true" ${rpcFile}
if [ $? -ne 0 ]; then
    echo "Failed to set client_export = true"
    exit 1
fi

echo "Building with stress login"
./nant -D:config=unix64-clang-debug -D:use_stress_login=true ${@}
if [ $? -ne 0 ]; then
    echo "Failed to successfully build the dirtycast code for debug..."
    setClientExport "false" ${rpcFile}
    exit 1
fi
./nant -D:config=unix64-clang-opt -D:use_stress_login=true ${@}
if [ $? -ne 0 ]; then
    echo "Failed to successfully build the dirtycast code for opt..."
    setClientExport "false" ${rpcFile}
    exit 1
fi

# Clean up changes
setClientExport "false" ${rpcFile}

# duplicate "run" directories into a new "buildoutput" directory to avoid losing it when build location is a mounted directory (externally synced source code scenario)
mkdir /tmp/EAGS/buildoutput
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/run /tmp/EAGS/buildoutput/run
mkdir /tmp/EAGS/buildoutput/GameServer
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/GameServer/run /tmp/EAGS/buildoutput/GameServer/run
mkdir /tmp/EAGS/buildoutput/qosstress
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/qosstress/run /tmp/EAGS/buildoutput/qosstress/run
mkdir /tmp/EAGS/buildoutput/stress
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/stress/run /tmp/EAGS/buildoutput/stress/run
