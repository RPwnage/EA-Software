#!/bin/bash

checkroot=1
warn=0

for opt in "$@"
do
case $opt in
  --skipcheck)
    checkroot=0;;
  --warn)
    warn=1;;
  *)
    ;;
esac
done

if [ $checkroot -ne 0 ]; then
  p4client=$(p4 set | grep -oE "[^ ]+" | grep "P4CLIENT" | grep -oE "[^{^P4CLIENT=$}]+")
  if [ -z "$p4client" ]; then
      echo "ERROR: P4CLIENT not set!"
      exit 1
  fi
fi

pushd ../../../ > /dev/null
blazeRoot=$PWD
canonical_blazeRoot=`readlink -f $blazeRoot` # use canonical path!
if [ $checkroot -ne 0 ]; then
  clientRoot=$(p4 info | grep "Client root: " | grep -oE "/.*")
  pushd $clientRoot > /dev/null
  p4root=$PWD
  canonical_p4root=`readlink -f $p4root` # use canonical path!
  if [[ ! "$canonical_blazeRoot" =~ ^$canonical_p4root ]]; then
      echo "ERROR: current directory is not in client root ($clientRoot)."
      exit 1
  fi
  popd > /dev/null
fi

popd > /dev/null


packageRoot=$P4_PACKAGE_ROOT
if [ -z "$packageRoot" ]; then
    # For vanilla Blaze, all packages are in one of two directories relative to the blazeserver folder:
    # ../ (containing EATDF, EATDF_Java, Framework), and the on-demand root ../../packages.
    # Thus, this script assumes that all package directories are located in a subdirectory of ../../
    # (relative to the blazeserver folder)
    pushd ../../../../ > /dev/null
    packageRoot=$PWD
    popd > /dev/null

    if [ -n "$1" ] && [ $warn -ne 0 ]; then
        echo "WARNING: Assuming that all nant packages are in a subdirectory of $packageRoot . If this is not the case, set the P4_PACKAGE_ROOT environment variable to a directory that is the parent of all your nant package directories before attempting to run this script."
    fi
fi

