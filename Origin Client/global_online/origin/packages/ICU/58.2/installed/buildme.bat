@echo off
::
:: setup Visual Studio 2013 environment for x86 builds
::
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
::
:: build
::
msbuild source\allinone\allinone.sln /m /target:Clean /property:Configuration=Release;Platform=Win32
msbuild source\allinone\allinone.sln /m /target:Clean /property:Configuration=Debug;Platform=Win32
msbuild source\allinone\allinone.sln /m /target:Build /property:Configuration=Release;Platform=Win32
msbuild source\allinone\allinone.sln /m /target:Build /property:Configuration=Debug;Platform=Win32
