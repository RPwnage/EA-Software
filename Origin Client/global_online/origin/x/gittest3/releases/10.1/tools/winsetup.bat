SET orig_cwd=%CD%

cd ..\jssdk
call npm install
call bower install
call grunt
if %ERRORLEVEL% NEQ 0 GOTO exit
call bower link

cd ..\i18n
call npm install
call bower install
call grunt
if %ERRORLEVEL% NEQ 0 GOTO exit
call bower link

cd ..\components
call npm install
call bower link origin-jssdk
call bower link origin-i18n
call bower install
call grunt
if %ERRORLEVEL% NEQ 0 GOTO exit
call bower link

cd ..\app
call npm install
call bower link origin-jssdk
call bower link origin-i18n
call bower link origin-components
call bower install

cd ..\automation\minispa
call npm install
call bower link origin-jssdk
call bower link origin-i18n
call bower link origin-components
call bower install

:exit
cd %orig_cwd%