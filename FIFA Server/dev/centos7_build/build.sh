#! /bin/sh

docker build --force-rm=true --no-cache=false --build-arg BASE_IMAGE=$(cat ../centos7_base/TAG) -t $(cat TAG) .
