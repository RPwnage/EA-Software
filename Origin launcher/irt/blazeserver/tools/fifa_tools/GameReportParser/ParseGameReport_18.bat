@echo on
set ROOT=D:\p4serv1\gosdev\games\FIFA\2018\Gen4\dev\blazeserver\15.x\tools\fifa_tools\GameReportParser
set PATH=C:\Python27\;C:\Program Files\WinRaR;

call python GameReportParser.py  ./encrypted/FIFA18_ps4 ./gamereports ps4 FIFA18 "EADP"
pushd %ROOT%

pause