@echo on
set ROOT=%cd%
set PATH=C:\Python27\;C:\Program Files (x86)\WinRaR;

call python GameReportParser.py ./encrypted/FIFA15_ps4 ./gamereports ps4 FIFA15 "EADP"
pushd %ROOT%

pause