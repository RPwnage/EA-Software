#SERVERIP="127.0.0.1"
SERVERIP="172.16.0.10"

LIFETIME="0"
./loadtestclient $SERVERIP:22990 100000-115000 -tf 0 -cr 1000 -ct 0 -loss 0 -upk -et 60000 &
./loadtestclient $SERVERIP:22990 200000-215000 -tf 0 -cr 1000 -ct 0 -loss 0 -upk -et 60000 &
./loadtestclient $SERVERIP:22990 300000-315000 -tf 0 -cr 1000 -ct 0 -loss 0 -upk -et 60000 &
