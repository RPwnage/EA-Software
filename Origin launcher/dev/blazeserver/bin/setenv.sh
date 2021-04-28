#!/bin/bash

#
# Set the PATH and LD_LIBARY_PATH for running blaze executables
#
# By default the unix64-clang-debug build config is used for setting up the path.  This
# can be overridden by setting the BLAZE_BUILD_CONFIG environment variable prior to sourcing
# this script.
#

ROOT_DIR=$(cd $(dirname $BASH_SOURCE)/.. >/dev/null 2>&1; pwd)
PACKAGE_VERSION=$(basename $(cd $ROOT_DIR/build/blazeserver/* >/dev/null 2>&1; pwd))

if [ -z "$BLAZE_BUILD_CONFIG" ]; then
    BLAZE_BUILD_CONFIG=unix64-clang-debug
fi

BLAZE_PATH=$ROOT_DIR/build/blazeserver/$PACKAGE_VERSION/$BLAZE_BUILD_CONFIG/bin
BLAZE_PATH=$BLAZE_PATH:$ROOT_DIR/bin

# Add bin directory to user path if not already present
echo $PATH | grep $BLAZE_PATH > /dev/null 2>&1
if [ $? != 0 ]; then
    export PATH=$BLAZE_PATH:$PATH
fi

BLAZE_LD_LIBRARY_PATH=$ROOT_DIR/build/blazeserver/$PACKAGE_VERSION/$BLAZE_BUILD_CONFIG/container_lib:$ROOT_DIR/lib
echo $LD_LIBRARY_PATH | grep $BLAZE_LD_LIBRARY_PATH > /dev/null 2>&1
if [ $? != 0 ]; then
    export LD_LIBRARY_PATH=$BLAZE_LD_LIBRARY_PATH:$LD_LIBRARY_PATH
fi
