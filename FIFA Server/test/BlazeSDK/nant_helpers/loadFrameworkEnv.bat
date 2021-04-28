@REM this is a one-stop shop for loading up the current framework2 environment paths/vars, etc
set NANT_VERSION="3.25.00"

@REM if you use a non-standard package dir, you can set PACKAGE_DIR
if NOT DEFINED PACKAGE_DIR set PACKAGE_DIR=c:\packages

call %PACKAGE_DIR%\Framework\%NANT_VERSION%\bin\NAntenv.bat "%PACKAGE_DIR%\Framework\%NANT_VERSION%\bin"

