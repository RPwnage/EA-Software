#! /bin/bash

username=$1
devuid=$(id -u $username)
devgid=$(id -g $username)
devgroup=$(id -g -n $username)

if [ -z "$devuid" ] || [ -z "$devgid" ] || [ -z "$devgroup" ]; then
  echo "ERROR: Missing info for user '$username':"
  echo "       uid: $devuid"
  echo "       group: $devgroup"
  echo "       gid: $devgid"
  exit 1
fi
