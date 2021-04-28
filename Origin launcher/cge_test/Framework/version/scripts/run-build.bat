@echo off

REM first parameter is optionally path to the masterconfig
REM second parameter is optionally the root folder of the Framework Package:

if "%1" == "" (
    if "%TNT_ROOT%" == "" (
        SET masterconfig_path=%~dp0..\..\..\masterconfig.xml
    ) else (
        SET masterconfig_path=%TNT_ROOT%\masterconfig.xml
    )
) else (
    SET masterconfig_path=%1
)

if "%2" == "" (
    if "%TNT_ROOT%" == "" (
        SET eapm_path=%~dp0..\bin\eapm.exe
		SET framework_sln_path=%~dp0..\framework.sln
    ) else (
        SET eapm_path=%TNT_ROOT%\Build\Framework\bin\eapm.exe
		SET framework_sln_path=%TNT_ROOT%\Build\Framework\framework.sln
    )
) else (
    SET eapm_path=%2\bin\eapm.exe
	SET framework_sln_path=%2\framework.sln
)

%eapm_path% install MSBuildTools -masterconfigfile:%masterconfig_path%
REM Find where the package got installed:
for /f %%i in ('%eapm_path% where MSBuildTools -masterconfigfile:%masterconfig_path%') do set MSBUILDTOOLS_PACKAGE=%%i
echo Extracted MSBuildTools package path: %MSBUILDTOOLS_PACKAGE%

%eapm_path% install DotNet -masterconfigfile:%masterconfig_path% -G:package.DotNet.use-non-proxy=true
REM Find where the package got installed:
for /f %%i in ('%eapm_path% where DotNet -masterconfigfile:%masterconfig_path% -G:package.DotNet.use-non-proxy^=true') do set DOTNETFRAMEWORKSDK_PACKAGE=%%i
echo Extracted DotNet package path: %DOTNETFRAMEWORKSDK_PACKAGE%

%eapm_path% install DotNetCoreSdk -masterconfigfile:%masterconfig_path%
REM Find where the package got installed:
for /f %%i in ('%eapm_path% where DotNetCoreSdk -masterconfigfile:%masterconfig_path%') do set DOTNETCORESDK_PACKAGE=%%i
echo Extracted DotNetCoreSdk package path: %DOTNETCORESDK_PACKAGE%

REM non-proxy package only has one SDK folder in it so use FOR DIR to get it and store it
FOR /F "delims=" %%i IN ('dir %DOTNETCORESDK_PACKAGE%\Installed\dotnet\sdk\ /b') DO SET DOTNETCORESDK_PACKAGE_VERSION=%%i

REM for /F tokens^=2^,3^,5delims^=^<^"^= %%a in ('findstr /I /C:package.DotNet.frameworkversion %DOTNETFRAMEWORKSDK_PACKAGE%\scripts\initialize.xml') do (
   REM if "%%a" equ "property name" if "%%b" equ "package.DotNet.frameworkversion" set DOTNETFRAMEWORKSDK_PACKAGE_VERSION=%%c
REM )

REM need to set this up so that MSBuild can find the dotnet core SDK:
set DOTNET_MSBUILD_SDK_RESOLVER_SDKS_DIR=%DOTNETCORESDK_PACKAGE%\Installed\dotnet\sdk\%DOTNETCORESDK_PACKAGE_VERSION%\Sdks
set TARGETFRAMEWORKROOTPATH=%DOTNETFRAMEWORKSDK_PACKAGE%/installed/Reference Assemblies

if exist TnT\\Build\\Framework\\bin ( 
	rd /Q /S TnT\\Build\\Framework\\bin
)

%MSBUILDTOOLS_PACKAGE%\BuildTools\Common7\Tools\VsDevCmd.bat && msbuild %framework_sln_path% /r /nodeReuse:false /v:m /t:Build /m /clp:DisableConsoleColor /p:Configuration=Release