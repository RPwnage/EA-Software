@echo off
IF [%1]==[] GOTO usage
SET "pool=%~2"
SETLOCAL enabledelayedexpansion

@REM Set default pool if not provided
IF "!pool!"=="" ( SET pool="GS EARS Dev" )

@REM Derive the instance name from username and the forwarding port.
SET instanceName=%USERNAME%%1
@REM Reserve an instance
ggp instance reserve --pool %pool% --name %instanceName%
IF %ERRORLEVEL% NEQ 0 (
  ECHO Failed to reserve an instance - %instanceName%.
  PAUSE
  EXIT /B 1
)

ECHO Reserved an instance - %instanceName% 


@REM - Take the first IP we find and use it for mapping later
for /f "tokens=1-2 delims=:" %%a in ('ipconfig ^|find "IPv4"') do (
set ipaddr=%%b
set ipaddr=!ipaddr: =!
goto port_mapping
)

:port_mapping
ECHO Setting SSH Forward port mapping
@REM https://developers.google.com/stadia/1.41/docs/develop/tools-and-integrations/ssh-port-forwarding?authuser=1
START /MIN "PyroForward%1" ggp ssh shell --ssh-flag "-nNT" --ssh-flag "-L %1:localhost:6363" --instance %instanceName%


ECHO Setting SSH Reverse port mapping to local IP %ipaddr%

ECHO coreSlave1 at port 11000
START /MIN "ServerReverse%1" ggp ssh shell --ssh-flag "-nNT" --ssh-flag "-R 11000:%ipaddr%:11000" --instance %instanceName%

ECHO coreSlave2 at port 11020
START /MIN "ServerReverse%1" ggp ssh shell --ssh-flag "-nNT" --ssh-flag "-R 11020:%ipaddr%:11020" --instance %instanceName%

:release
ECHO Press any key to release the instance %instanceName%.... 
PAUSE >nul

setlocal
:PROMPT
SET /P AREYOUSURE=Are you sure (y/[n])?
IF /I "%AREYOUSURE%" NEQ "y" GOTO release
endlocal


taskkill /FI "WindowTitle eq PyroForward%1*" /T /F
taskkill /FI "WindowTitle eq ServerReverse%1*" /T /F

@REM Release the instance
ggp instance release %instanceName%
IF %ERRORLEVEL% NEQ 0 (
  ECHO Failed to release the instance - %instanceName%.
  PAUSE
  EXIT /B 1
)
EXIT /B 1



:usage
ECHO Insufficient Arguments
ECHO Usage: %0 ^<PortNum^> ^<Pool(optional)^>
PAUSE
EXIT /B 1