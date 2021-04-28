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
		SET framework_test_assembly=%~dp0..\Local\bin\test\NAnt.UnitTests.dll
    ) else (
        SET eapm_path=%TNT_ROOT%\Build\Framework\bin\eapm.exe
		SET framework_test_assembly=%TNT_ROOT%\Build\Framework\Local\bin\test\NAnt.UnitTests.dll
    )
) else (
    SET eapm_path=%1\bin\eapm.exe
	SET framework_test_assembly=%1\Local\bin\test\NAnt.UnitTests.dll
)


%eapm_path% install MSBuildTools -masterconfigfile:%masterconfig_path%
REM Find where the package got installed:
for /f %%i in ('%eapm_path% where MSBuildTools -masterconfigfile:%masterconfig_path%') do set MSBUILDTOOLS_PACKAGE=%%i

%MSBUILDTOOLS_PACKAGE%\BuildTools\Common7\IDE\Extensions\TestPlatform\vstest.console.exe %framework_test_assembly%