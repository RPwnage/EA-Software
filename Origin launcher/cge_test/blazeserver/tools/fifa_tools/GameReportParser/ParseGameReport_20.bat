@echo on
set ROOT=D:\p4serv1\gosdev\games\FIFA\2020\Gen4\dev\blazeserver\tools\fifa_tools\GameReportParser
set PATH=C:\Python27\;C:\Program Files\WinRaR;C:\Program Files\7-Zip;

call python GameReportParser.py  .\encrypted\FIFA20_ps4 .\gamereports ps4 FIFA20 "EADP"
pushd %ROOT%

pause

