#!/bin/bash
# bash script automating debug tools installation and removal

if [[ $# -gt 0 && ($1 == "uninstall" || $1 == "clean") ]]; then
  echo "Uninstalling debug tools"
  apt-get -y remove net-tools
  apt-get -y remove tcpdump
  apt-get -y remove gdb
  apt-get -y remove strace
else
  echo "Installing debug tools"
  apt-get -y install net-tools
  apt-get -y install tcpdump
  apt-get -y install gdb
  apt-get -y install strace
fi
