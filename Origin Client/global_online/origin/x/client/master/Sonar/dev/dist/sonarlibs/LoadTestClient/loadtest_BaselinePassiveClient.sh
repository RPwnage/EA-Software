#SERVERIP="127.0.0.1"
SERVERIP="192.168.21.11" 
LIFETIME="86400"
./loadtestclient $SERVERIP:22990 0-1 -tf 0 -cr 100 -ct 0 -loss 0
