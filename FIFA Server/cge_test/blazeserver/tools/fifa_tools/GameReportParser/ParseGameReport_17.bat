@echo on
set ROOT=D:\p4serv1\gosdev\games\FIFA\2017\Gen4\dev\blazeserver\15.x\tools\fifa_tools\GameReportParser
set PATH=C:\Python27\;C:\Program Files (x86)\WinRaR;

call python GameReportParser.py  ./encrypted/FIFA17_ps4 ./gamereports ps4 FIFA17 "EADP"
pushd %ROOT%

pause