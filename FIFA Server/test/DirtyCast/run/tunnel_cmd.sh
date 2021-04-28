#!/bin/bash

VERSION=`/opt/ea/stunnel/bin/stunnel -version 2>&1 | head -n1`
SERVERS=`grep -s 'voipserver' stunnel.cfg | wc -l`
LOGFILE=`grep -s 'output' stunnel.cfg | sed 's/^.* = //'`
DATE=`date +%Y-%m-%dT%H-%S-%3N`
PORTS=`grep -s 'connect' stunnel.cfg | awk -v ORS=, -F'= ' '{print $2}' | sed 's/,$/\n/'`

if [ $# -lt 1 ]
then
    echo "Usage: $0 <command>"
    exit
fi

shopt -s nocasematch

case "$1" in
info)
    if [ -f stunnel.pid ]; then
        echo '<servulator>'
        echo '  <Init>true</Init>'
        echo '  <Name>stunnel</Name>'
        echo "  <Version>$VERSION</Version>"
        echo "  <Port>$PORTS</Port>"
        echo '  <State>na</State>'
        echo '  <GameMode>na</GameMode>'
        echo '  <Ranked>na</Ranked>'
        echo '  <Private>na</Private>'
        echo "  <Players>$SERVERS</Players>"
        echo '  <Observers>0</Observers>'
        echo '  <PlayerList>na</PlayerList>'
        echo '  <ObserverList>na</ObserverList>'
        echo '</servulator>'
    fi
    ;;
stop)
    if [ -f stunnel.pid ]; then
        kill -TERM `cat stunnel.pid`

        # if pid still exists rm
        sleep 1
        if [ -f stunnel.pid ]; then
            rm stunnel.pid
        fi

        # remove generated config
        rm stunnel.cfg
    fi
    ;;
reload)
    if [ -f stunnel.pid ]; then
        # reload the ports configuration
        ./start_tunnel.py --ports $2 --gen_only
        kill -HUP `cat stunnel.pid`
    fi
    ;;
rotate)
    if [ -f stunnel.pid ]; then
        # move the log and have stunnel reopen (touch to create again)
        mv $LOGFILE $LOGFILE-$DATE
        touch -a $LOGFILE
        kill -USR1 `cat stunnel.pid`
    fi
    ;;
*)
    echo "command $1 not processed"
    ;;
esac

shopt -u nocasematch
