@echo on
set ROOT=E:\p4serv1\gosdev\games\FIFA\2016\Gen4\dev\blazeserver\15.x\tools\fifa_tools\GameReportParser
set PATH=C:\Python27\;C:\Program Files (x86)\WinRaR;

call python GameReportParser.py  ./encrypted/FIFA16_ps4_beta ./gamereports ps4 FIFA16 CSV
pushd %ROOT%

pause