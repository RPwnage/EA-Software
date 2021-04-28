REM Build the visual studio solution file for the SDK.
call loadFrameworkEnv.bat
cd ..

nant buildtools
@if %ERRORLEVEL% NEQ 0 goto end_error

nant cleangeneratedcode
@if %ERRORLEVEL% NEQ 0 goto end_error

nant generatecode
@if %ERRORLEVEL% NEQ 0 goto end_error

@REM we build ps3 first, since that's most likely to have issues
nant -D:config="ps3-gcc-dev-debug" -D:samples=true -D:bulkbuild=false
@if %ERRORLEVEL% NEQ 0  goto ps3_fail

nant -D:config="pc-vc-dev-debug" -D:samples=true -D:bulkbuild=false
@if %ERRORLEVEL% NEQ 0  goto pc_fail

nant -D:config="xenon-vc-dev-debug" -D:samples=true -D:bulkbuild=false
@if %ERRORLEVEL% NEQ 0  goto xenon_fail
@goto end


:pc_fail
@echo "PC build failed"
@goto end

:xenon_fail
@echo "Xenon build failed"
@goto end

:ps3_fail
@echo "PS3 build failed"
@goto end

:end_error
@echo Error: nant returned error code %ERRORLEVEL%!
:end
pause