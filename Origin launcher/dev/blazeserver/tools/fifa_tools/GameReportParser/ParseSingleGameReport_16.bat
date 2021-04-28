@echo on
set ROOT=%cd%
set PATH=C:\Python27\;

call python ParseSingleReport.py ./encrypted/FIFA16_ps4 fifa-2016-ps4_blaze_grSlave7_gamereporting_20150727_235959-999.log
pushd %ROOT%

pause