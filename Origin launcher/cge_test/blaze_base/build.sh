#! /bin/sh

docker build --pull=true --force-rm=true --no-cache=true -t $(cat TAG) .
