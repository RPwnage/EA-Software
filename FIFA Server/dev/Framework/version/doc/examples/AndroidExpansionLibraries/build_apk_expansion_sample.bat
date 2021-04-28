@echo off
SET NANT_INPUT_OPTIONS=-masterconfigfile:%~dp0\..\masterconfig.xml 



echo -                                                                 
echo ------------------------------------------------------------------
echo -- Generate Visual Studio solution for tests: slntest target    --
echo ------------------------------------------------------------------
echo -                                                                 
%~dp0\..\..\..\bin\nant %NANT_INPUT_OPTIONS% -buildroot:%~dp0\..\..\build_incredibuild -buildfile:%~dp0\play_apk_expansion_sample\1.00.00\play_apk_expansion_sample.build  -D:config=android-gcc-dev-debug -D:package.configs=android-gcc-dev-debug slnruntime incredibuild

REM %~dp0\..\..\..\bin\nant %NANT_INPUT_OPTIONS% -buildroot:%~dp0\..\..\build_incredibuild -buildfile:%~dp0\play_apk_expansion_sample\1.00.00\play_apk_expansion_sample.build  -D:config=android-gcc-dev-debug -D:package.configs=android-gcc-dev-debug run-fast


echo -                                                                 
echo ------------------------------------------------------------------
echo -- Generate Visual Studio solution for tests: slntest target    --
echo ------------------------------------------------------------------
echo -                                                                 
%~dp0\..\..\..\bin\nant %NANT_INPUT_OPTIONS% -buildroot:%~dp0\..\..\build_nant -buildfile:%~dp0\play_apk_expansion_sample\1.00.00\play_apk_expansion_sample.build  -D:config=android-gcc-dev-debug  build

REM %~dp0\..\..\..\bin\nant %NANT_INPUT_OPTIONS% -buildroot:%~dp0\..\..\build_nant -buildfile:%~dp0\play_apk_expansion_sample\1.00.00\play_apk_expansion_sample.build  -D:config=android-gcc-dev-debug  run-fast

