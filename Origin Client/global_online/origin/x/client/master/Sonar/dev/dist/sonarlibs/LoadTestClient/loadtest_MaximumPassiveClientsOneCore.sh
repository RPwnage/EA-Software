#SERVERIP="127.0.0.1"
SERVERIP="172.16.0.10"

LIFETIME="0"
./loadtestclient $SERVERIP:22990 510000-514000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 520000-524000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 530000-534000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 540000-544000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &
./loadtestclient $SERVERIP:22990 550000-554000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &
#./loadtestclient $SERVERIP:22990 560000-564000 -tf 0 -cr 100 -ct $LIFETIME -loss 0 &

