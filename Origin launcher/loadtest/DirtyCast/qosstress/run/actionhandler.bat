echo off

if %1% == COMMAND (
 echo "no COMMAND"
)

if %1% == INFO (
 type servulatorInfo.xml 
 echo "completed INFO"
)

if %1% == CRASH (
 echo "no CRASH"
)


if %1% == DRAIN (
 Taskkill /PID %2 /F
 echo "completed DRAIN"
)

if %1% == STOP (
 Taskkill /PID %2 /F
 echo "completed STOP"
)