REM Build the visual studio solution file for the SDK.
call loadFrameworkEnv.bat
cd ..

nant buildtools
@if %ERRORLEVEL% NEQ 0 goto end_error
nant cleangeneratedcode
@if %ERRORLEVEL% NEQ 0 goto end_error
nant generatecode
@if %ERRORLEVEL% NEQ 0 goto end_error

nant package
@if %ERRORLEVEL% NEQ 0 goto end_error
@goto end

:end_error
echo Error: nant returned error code %ERRORLEVEL%!
pause
:end
