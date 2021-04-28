#!/bin/bash

if [ -n "$1" ] && [ "$1" = "--help" ]; then
  echo "Usage: ./setup.sh [-f|--force] [-t <build_target>]"
  echo "This script generates the blaze tarball and sets the environment variables required to run the blazeserver locally in Docker Compose."
  echo "The docker base image is determined by the OS in which the local blazeserver was built."
  exit 0
fi

force=0
target="debug"
readtarget=0
for opt in "$@"
do
  if [ $readtarget -ne 0 ]; then
    target=$opt
    readtarget=0
  else
    case $opt in
    -f|--force)
      force=1;;
    -t)
      readtarget=1;;
    *)
      ;;
    esac
  fi
done

if [ $(ls archive | grep "\.tar\.gz$" | wc -l) -eq 0 ]; then
  force=1
fi

if [ -f .env ]; then
  if [ $force -ne 0 ]; then
    . .env
    imgid=$(docker images -q $DC_BLAZE_IMAGE)
    if [ -n "$imgid" ]; then
      echo "INFO: Attempting to remove existing image '$DC_BLAZE_IMAGE'"
      docker rmi -f $imgid
    fi
  fi
fi

. ../getuserinfo.sh $USER
. ../getp4info.sh

if [ $force -ne 0 ]; then
  rm -rf ../../out
  rm -rf archive/blaze*
  ../dev/run.sh -c ./nant staging
  mkdir -p archive/blaze/bin
  mkdir archive/blaze/lib
  cp -r ../../bin/dbmig archive/blaze/bin/
  cp ../../out/linux/$target/bin/deployimage archive/blaze/bin/
  cp ../../out/linux/$target/bin/blazeserver archive/blaze/bin/
  cp ../../out/linux/$target/lib/* archive/blaze/lib/

  pushd archive/blaze >/dev/null
  tar -zcvf ../blaze.tar.gz . 
  popd >/dev/null
fi

baseimg=$(tar xfO archive/*.tar.gz ./bin/deployimage)
if [ -z "$baseimg" ]; then
  echo "ERROR: Unable to determine base docker image for blazeserver tarball."
  exit -1
fi

echo "DC_UID=$devuid" > .env
echo "DC_GID=$devgid" >> .env
echo "DC_BLAZE_BASE_DIR=$blazeRoot/blazeserver" >> .env
echo "DC_BLAZE_BASE_IMAGE=$baseimg" >> .env
echo "DC_BLAZE_IMAGE=$p4client-test" >> .env
echo "DC_REDIS_IMAGE=$(cat ../redis/TAG)" >> .env
echo "DC_MYSQL_IMAGE=$(cat ../mysql/TAG)" >> .env
