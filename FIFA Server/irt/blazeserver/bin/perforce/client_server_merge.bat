@REM Server to Client integration using three branch mappings
@echo OFF
echo.

::Storing current workspace in a variable
p4 -F "%%Client%%" -ztag client -o>current_client.txt
if %errorlevel% neq 0 (echo "p4 getting current Client failed, exiting..." & PAUSE & exit /b)
for /f "tokens=1" %%a in (current_client.txt) do set current_client=%%a
del current_client.txt

::Optional argument for P4CLIENT. Defaults to current workspace if no argument is provided. 
if not "%1"=="" (p4 set P4CLIENT=%1)
if %errorlevel% neq 0 (echo "p4 setting client failed, exiting..." & PAUSE & exit /b) 

::Get the current stream name. 
p4 -F "%%Name%%" -ztag stream -o>current_stream.txt
if %errorlevel% neq 0 (echo "p4 getting Stream from Client failed, exiting..." & PAUSE & exit /b)
for /f "tokens=1" %%a in (current_stream.txt) do set current_stream=%%a
del current_stream.txt
 
::The following values can be modified to use specific branchspec. 
set "STREAM=//gos-stream/%current_stream%"
set "CORE_BRANCHSPEC=client-server-share-blaze-core-%current_stream%"
set "PROXY_BRANCHSPEC=client-server-share-blaze-proxy-%current_stream%"
set "CUSTOM_BRANCHSPEC=client-server-share-blaze-custom-%current_stream%"

::Make approriate changes to the branchspec_list as well to include/exclude any branchspecs.
set branchspec_list=%CORE_BRANCHSPEC% %PROXY_BRANCHSPEC% %CUSTOM_BRANCHSPEC%

set "changes=0"

p4 --field "Description=[nobug][build]Merging using client-server-share-blaze branch mappings" --field "Files=" change -o | p4 -Ztag -F "%%change%%" change -i>cl_number.txt
if %errorlevel% neq 0 (echo "p4 cl creation failed, exiting..." & PAUSE & exit /b)
for /f "tokens=2" %%a in (cl_number.txt) do set cl_number=%%a
del cl_number.txt

for %%a in (%branchspec_list%) do (
    p4 integrate -c %cl_number% -b %%a > %%a_output_file.txt 2>&1
    type %%a_output_file.txt
    if %errorlevel% neq 0 (echo "p4 integrate failed, exiting..." & PAUSE & exit /b)
    for /f "tokens=4" %%b in (%%a_output_file.txt) do (
        if not "%%b"=="integrated." (
            set "changes=1"
        )
    )
    del %%a_output_file.txt
)

if %changes%==1 (
    p4 resolve -am -c %cl_number%
    echo.
    echo Added changes to CL %cl_number%
) else (
    p4 -q change -d %cl_number%
)

p4 set P4CLIENT=%current_client%

echo.
PAUSE
