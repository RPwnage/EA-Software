@echo on
PUSHD ..\..\..\..\..\..\..\
set ROOT=%CD%
POPD

set PATH=%ROOT%\packages\common\Framework\3.25.00\bin;

rem call nant slnruntime

call nant build -D:config=pc64-vc-dev-debug >buildLog.txt


