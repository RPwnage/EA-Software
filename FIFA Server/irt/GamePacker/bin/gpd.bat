@ECHO OFF
SETLOCAL
CALL %~dp0\_setGP_vars.bat

@REM note %% necessary to escape %

@ECHO ON
pushd . & chdir %GP_ROOT% & %GP_PATH%\GamePackerTool.exe %* & popd