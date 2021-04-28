@echo on
PUSHD ..\..\..\..\..\..\..
set ROOT=%CD%
POPD

set PATH=%ROOT%\packages\common\Framework\2.15.14\bin;

call nant clean