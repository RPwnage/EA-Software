@echo off
setlocal ENABLEDELAYEDEXPANSION

:: Run Blaze Server
:: =================================
:: Runs the Blaze Server.
:: ---------------------------------

:SETUP
:: ---------------------------------
:: Find the local path of this batch file.
:: ---------------------------------
set LOCAL_PATH=%~dp0
set LOCAL_CMD=%~n0
goto START

:USAGE
:: ---------------------------------
:: Usage
:: ---------------------------------
:: No or wrong parameters. Show usage

echo.
echo BlazeServer (Tool)
echo ----------------------------
echo Will run 1 or more copies of the server in 1 go.
echo.
echo Will open new windows and terminate them when server dies by default.
echo.
echo Usage:
echo   %LOCAL_CMD% [options] [debug^|release] ^<config1.boot^> [^<config2.boot^>] [...]
echo.
echo o [options]
echo    /c /current - run in current window instead of new window.
echo            Only 1 config allowed.
echo    /i /ignore - continue even if specified config files not present.
echo            (Will not start missing instance(s).)
echo    /k /keepalive - Do not terminate new windows after server has stopped.
echo.
echo o debug - runs debug version of the server [default].
echo.
echo o release - runs release version of the server.
echo.
echo o configX.file defines the configuration settings for server "X".
echo.

goto QUIT

:START
:: ---------------------------------
:: Start
:: ---------------------------------
:: Set defaults.
set LOCAL_MODE=debug
set LOCAL_CONFIG_COUNT=0
set LOCAL_CONFIG_LIST=
set LOCAL_CONFIG_MISS=
set LOCAL_NEW_WINDOW=Y
set LOCAL_NO_TERMINATE_WHEN_END=N
set LOCAL_IGNORE_START=N
set LOCAL_IGNORE_RUN_ONE=N
set LOCAL_DESTRUCTIVE=


:PARSEPARAMS
:: ---------------------------------
:: Parse parameters.
:: ---------------------------------

if "%1" == "" goto CHECKPARAMS

if /i "%1" EQU "debug" (
    set LOCAL_MODE=debug
) else if /i "%1" EQU "release" (
    set LOCAL_MODE=release
) else if /i "%1" EQU "--dbdestructive" (
    set LOCAL_DESTRUCTIVE=--dbdestructive
) else if /i "%1" EQU "/current" (
    set LOCAL_NEW_WINDOW=N
) else if /i "%1" EQU "/c" (
    set LOCAL_NEW_WINDOW=N
) else if /i "%1" EQU "/ignore" (
    set LOCAL_IGNORE_START=Y
) else if /i "%1" EQU "/i" (
    set LOCAL_IGNORE_START=Y
) else if /i "%1" EQU "/keepalive" (
    set LOCAL_NO_TERMINATE_WHEN_END=Y
) else if /i "%1" EQU "/k" (
    set LOCAL_NO_TERMINATE_WHEN_END=Y
) else (
    :: Remember the list of property files.
    if exist %1 (
        set LOCAL_CONFIG_LIST=%LOCAL_CONFIG_LIST% %1
        set /A LOCAL_CONFIG_COUNT+=1
    ) else (
        set LOCAL_CONFIG_MISS=!LOCAL_CONFIG_MISS! %1
    )
)

shift
goto PARSEPARAMS


:CHECKPARAMS
:: ---------------------------------
:: Check parameters.
:: ---------------------------------
echo.
echo Starting...
echo ---------------------
echo Mode: %LOCAL_MODE%
echo [Debug] LOCAL_CONFIG_LIST = %LOCAL_CONFIG_LIST%
echo [Debug] LOCAL_CONFIG_MISS = %LOCAL_CONFIG_MISS%
echo [Debug] LOCAL_DESTRUCTIVE = %LOCAL_DESTRUCTIVE%
echo ---------------------
echo.

:: If there are missing config files, see what to do about it.
if not "%LOCAL_CONFIG_MISS%"=="" (
    echo Missing config files: %LOCAL_CONFIG_MISS%
    if "%LOCAL_IGNORE_START%"=="Y" (
        echo Ignoring missing config files.
    ) else (
        set USER_ANSWER="N"
        set /p USER_ANSWER="continue? [y/N] "
        if /i "!USER_ANSWER!" EQU "y" (
            set LOCAL_IGNORE_START=Y
        ) else if /i "!USER_ANSWER!" EQU "yes" (
            set LOCAL_IGNORE_START=Y
        ) else (
            echo Quitting...
            goto QUIT
        )
    )
)

:: If there is not at least one config, show usage and quit.
if "%LOCAL_CONFIG_LIST%"=="" (
    echo No config files specified.
    goto USAGE
)

:: If there are more than 1 config file, but the user wants to run in
:: the current window, find out what to do.
if /i "%LOCAL_NEW_WINDOW%"=="N" (
    echo Starting server in local window...

    if LOCAL_CONFIG_COUNT GTR 1 (
        set USER_ANSWER="N"
        echo You are trying to run %LOCAL_CONFIG_COUNT% servers in current window when only 1 allowed.
        set /p USER_ANSWER="continue (to run just first config file)? [y/N] "
        echo USER_ANSWER=!USER_ANSWER!
        if /i "!USER_ANSWER!" EQU "y" (
            set LOCAL_IGNORE_RUN_ONE=Y
        ) else if /i "!USER_ANSWER!" EQU "yes" (
            set LOCAL_IGNORE_RUN_ONE=Y
        ) else (
            echo Quitting...
            goto QUIT
        )
    )
)

:RUN
:: ---------------------------------
:: Run the servers.
:: ---------------------------------

for %%* in (..) do set LOCAL_VERSION_DIR=%%~nx*

for %%C in (%LOCAL_CONFIG_LIST%) do (
    echo Running BlazeServer w/ %%C
    if "%LOCAL_NEW_WINDOW%"=="N" (
        cmd /c ..\build\blazeserver\%LOCAL_VERSION_DIR%\pc64-vc-%LOCAL_MODE%\bin\blazeserver %LOCAL_DESTRUCTIVE% %%C
        goto QUIT
    ) else if "%LOCAL_NO_TERMINATE_WHEN_END%"=="Y" (
        start "%%C" cmd /k ..\build\blazeserver\%LOCAL_VERSION_DIR%\pc64-vc-%LOCAL_MODE%\bin\blazeserver %LOCAL_DESTRUCTIVE% %%C
    ) else (
        start "%%C" ..\build\blazeserver\%LOCAL_VERSION_DIR%\pc64-vc-%LOCAL_MODE%\bin\blazeserver %LOCAL_DESTRUCTIVE% %%C
    )
)

:QUIT
endlocal
