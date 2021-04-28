@echo on
set ROOT=%cd%
set PATH=C:\Python27\;

call python ParseSingleReport.py ./encrypted/FIFA15_ps4 fifa-2015-ps4_blaze_coreSlave63_gamereporting_20150325_215453-598.log
pushd %ROOT%

pause