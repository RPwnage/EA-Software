#!/bin/bash
# bash script automating sync and build phase.
#    * sync
#    * build
. ./sync.sh

# mclouatre jan 14 2018  -  commented out - no longer required - can build successfully without this
# run framework's eapm.exe to specify credential for package server
# mono /tmp/EAGS/gos-stream/ml/Framework/version/bin/eapm.exe password

./build.sh
