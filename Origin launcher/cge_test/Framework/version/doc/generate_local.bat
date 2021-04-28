@echo off
pushd "%~dp0\..\source\hugodoc 
start /b cmd /c bin\hugo serve --ignoreCache
:loop
netstat -anp TCP | find /i "listening" | find ":1313" >nul 2>nul 
if %ERRORLEVEL% equ 0 goto started
echo Waiting for server to launch...
timeout /t 2 /nobreak > NUL
goto loop
:started
echo Server started, launching browser. Close this window to stop serving.
start "" "http://localhost:1313/framework/fwdocs/"
popd
