@echo off
set /p sandboxAddr="Enter SandBox Address: (leave empty to set as www)"
set /p spaVer="Enter Spa Version: (leave empty to set as alpha)"
set /p testEnv="Enter Test Enviroment: (leave empty to set as dev)"
set /p cmsStage="Enter CMS Stage: (leave empty to set as live)"
IF NOT DEFINED sandboxAddr (SET sandboxAddr=www)
IF NOT DEFINED spaVer (SET spaVer=alpha)
IF NOT DEFINED testEnv (SET testEnv=dev)
IF NOT DEFINED cmsStage (SET cmsStage=live)
setx /m SANDBOX_ADDRESS sandboxAddr
setx /m SPA_VERSION spaVer
setx /m TEST_ENVIRONMENT testEnv
setx /m CMSSTAGE cmsStage
IF NOT EXIST "C:\python27-seleniumgrid\python.exe" ( 
    bitsadmin /Transfer PythonDownload https://s3.amazonaws.com/client-automation/packages/python27-seleniumgrid.zip C:\python27-seleniumgrid.zip
    "C:\Program Files\7-Zip\7z.exe" x C:\python27-seleniumgrid.zip -oc:\
    IF NOT EXIST "C:\python27-seleniumgrid\python.exe" ( 
        Echo ERROR Python27 cannot be found 
        Pause
    )
)
IF NOT EXIST "C:\packages\Framework\3.19.00" ( 
    bitsadmin /Transfer FrameworkDownload http://packages.worldwide.ea.com/packages/Framework/Framework-3.19.00.zip C:\Framework-3.19.00.zip
    "C:\Program Files\7-Zip\7z.exe" x C:\Framework-3.19.00.zip -oc:\
    IF NOT EXIST "C:\Framework-3.19.00.exe" ( 
        Echo ERROR Framework cannot be found 
        Pause
    )
    start /d "C:\" Framework-3.19.00.exe
    Echo Press any key after you have installed Framework
)
Pause 
IF EXIST "C:\Framework-3.19.00.zip" ( 
    del C:\Framework-3.19.00.zip
)
IF EXIST "C:\Framework-3.19.00.exe" ( 
    del C:\Framework-3.19.00.exe
)
IF EXIST "C:\python27-seleniumgrid.zip" ( 
    del C:\python27-seleniumgrid.zip
)
cd %~dp0
"C:\packages\Framework\3.19.00\bin\nant" download-vxwebui
Pause
