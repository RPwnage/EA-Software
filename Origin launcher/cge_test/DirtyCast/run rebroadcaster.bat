@echo off

cd run
start "Voip server" voipserverd.exe voipserver_local.cfg

cd ..\GameServer\run
start "Game server" gameserverd.exe gameserver_local.cfg