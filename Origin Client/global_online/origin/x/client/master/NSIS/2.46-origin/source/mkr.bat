@echo off
if not exist .instdist goto scons
rm -rf .instdist
:scons
scons UNICODE=no dist | tee mkr.out
if exist .instdist goto copyfiles
:copyfiles
