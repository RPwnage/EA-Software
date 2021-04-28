#!/bin/bash
# bash script automating build phase.

# build
cd /tmp/EAGS/gos-stream/ml/DirtyCast

if [[ ( -z "$PKG_USER" ) || ( -z "$PKG_PASSWORD" ) ]]; then
    mono ../Framework/version/bin/eapm.exe credstore -use-package-server
else
    mono ../Framework/version/bin/eapm.exe credstore -user:$PKG_USER -password:$PKG_PASSWORD -use-package-server
fi

./nant -D:config=unix64-clang-debug ${@}
if [ $? -ne 0 ]; then
    echo "Failed to successfully build the dirtycast code for debug..."
    exit 1
fi
./nant -D:config=unix64-clang-opt ${@}
if [ $? -ne 0 ]; then
    echo "Failed to successfully build the dirtycast code for opt..."
    exit 1
fi

# duplicate "run" directories into a new "buildoutput" directory to avoid losing it when build location is a mounted directory (externally synced source code scenario)
mkdir /tmp/EAGS/buildoutput
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/run /tmp/EAGS/buildoutput/run
mkdir /tmp/EAGS/buildoutput/GameServer
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/GameServer/run /tmp/EAGS/buildoutput/GameServer/run
mkdir /tmp/EAGS/buildoutput/qosstress
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/qosstress/run /tmp/EAGS/buildoutput/qosstress/run
mkdir /tmp/EAGS/buildoutput/stress
cp -r /tmp/EAGS/gos-stream/ml/DirtyCast/stress/run /tmp/EAGS/buildoutput/stress/run
