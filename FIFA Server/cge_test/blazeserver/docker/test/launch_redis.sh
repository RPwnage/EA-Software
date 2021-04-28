#!/bin/bash

cid=$(cat /proc/self/cgroup | grep "^1:" | grep -oE "/docker/.*")
cName=$(curl http://localhost:4243/containers/${cid#/docker/}/json | jq -r '.Name')
cInfo=${cName#*_}
cNum=${cInfo#*_}
rport=$((6378+${cNum}))

redis-server /etc/redis.conf --port ${rport}
