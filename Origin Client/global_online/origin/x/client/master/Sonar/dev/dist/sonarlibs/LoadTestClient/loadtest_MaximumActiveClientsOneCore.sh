#SERVERIP="127.0.0.1"
SERVERIP="172.16.0.10"

LIFETIME="0"
./loadtestclient $SERVERIP:22990 110000-111000 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 120000-121000 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 130000-131000 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 140000-141000 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 150000-151000 -cr 100 -ct $LIFETIME -loss 0 &
#./loadtestclient $SERVERIP:22990 160000-161000 -cr 100 -ct $LIFETIME -loss 0 &
#./loadtestclient $SERVERIP:22990 170000-171000 -cr 100 -ct $LIFETIME -loss 0 &
#./loadtestclient $SERVERIP:22990 180000-181000 -cr 100 -ct $LIFETIME -loss 0 &

