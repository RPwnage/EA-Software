@echo on
set ROOT=D:\p4serv1\gosdev\games\FIFA\2019\Gen4\dev\blazeserver\15.x\tools\fifa_tools\GameReportParser
set PATH=C:\Python27\;C:\Program Files\WinRaR;C:\Program Files\7-Zip;

call python GameReportParser.py  .\encrypted\FIFA19_ps4 .\gamereports ps4 FIFA19 "CSV"
pushd %ROOT%

pause