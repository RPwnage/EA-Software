rem This will install the local.app cert in your Trusted Root Certs keystore.
@echo off
certutil.exe -addstore "Root" "localapp.crt"
IF ERRORLEVEL 1 goto failed
echo.
echo =-=-=-=- Success! Press any key to exit. =-=-=-=-=
echo.
pause
goto end

:failed
echo.
echo =-=-=-=-= Failed to install cerificate. Press any key to exit. =-=-=-=-=
echo.
pause

:end