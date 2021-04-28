---
title: Clang Compilation Database
weight: 213
---

This page describes the target used for generating a clang compilation database.

<a name="Section1"></a>
## Overview ##

A clang compilation database is a JSON file that consists of a list of command objects.
Each command object contains the name of the file, the working directory and the command line statement with arguments.

###### an example of a clang compilation database file ######

```
[
{"Directory":"D:/samples/helloworld/1.00.23/build/helloworld/1.00.23/android-arm-clang-dev-debug/bin",
"Command":"\"D:/SDK/AndroidNDK/dev/installed/toolchains/llvm-3.4/prebuilt/windows-x86_64/bin/clang++.exe\"
-Wall -gcc-toolchain \"D:\\SDK\\AndroidNDK\\dev/installed/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64\"
-target armv5te-none-linux-androideabi -MMD -Wno-trigraphs -Wreturn-type -Wunused-variable -Wno-write-strings -fvisibility=hidden
-fpic -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -fsigned-char -marm -march=armv5te -mtune=xscale
-msoft-float -mfpu=vfp -mfloat-abi=softfp -B\"D:\\SDK\\AndroidNDK\\dev/installed/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64/bin\"
-B\"D:\\SDK\\AndroidNDK\\dev/installed/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64/libexec/gcc/arm-linux-androideabi/4.8\"
-B\"D:\\SDK\\AndroidNDK\\dev/installed/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64/arm-linux-androideabi/bin\"
-Wno-tautological-constant-out-of-range-compare -Wno-unused-private-field -Wno-reserved-user-defined-literal
-Wno-implicit-exception-spec-mismatch -Wno-overloaded-virtual -no-canonical-prefixes -Wno-extern-c-compat
-Wno-error-deprecated-register -fno-rtti -fno-exceptions -Werror -Wno-error=deprecated-declarations -O0 -fno-strict-aliasing -g3
-std=c++11 -D__STDC_INT64__ -MMD -c -fno-common -fshort-wchar -fmessage-length=0 -fno-rtti -D__ARM_ARCH_5__ -D__STDC_CONSTANT_MACROS
-D__STDC_LIMIT_MACROS -D__WCHAR_MIN__=0 -D_DEBUG -DANDROID -DEA_DEBUG -DEA_PLATFORM_ANDROID
-I\"D:/samples/helloworld/1.00.23/include\" -I\"D:/samples/helloworld/1.00.23/source\"
-I\"D:/SDK/AndroidNDK/dev/installed/platforms/android-18/arch-arm/usr/include\"
-I\"D:/SDK/AndroidNDK/dev/installed/sources/cxx-stl/gnu-libstdc++/4.8/include\"
-I\"D:/SDK/AndroidNDK/dev/installed/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi/include\"
-I\"D:/SDK/AndroidNDK/dev/installed/sources/cxx-stl/gnu-libstdc++/4.8/include/backward\"
D:/samples/helloworld/1.00.23/source/test.cpp",
"File":"D:/samples/helloworld/1.00.23/source/test.cpp"}
,
]
```
The easiest way to generate a clang compilation database is to use the target &quot;clang-compilation-database&quot;.

#### Target to Generate a Clang Compilation Database JSON File ####
Target |Standard |Short description |
--- |--- |--- |
| clang-compilation-database | ![requirements 1a]( requirements1a.gif ) | This target generates a clang compilation database JSON file. |

The behavior of the target can be adjusted using the following properties:

#### Clang Compilation Database Target Configuration ####
Property |Default value |Description |
--- |--- |--- |
| clangcompiledatabase.outputdir | ${nant.project.buildroot}/buildinfo/compilecommands | The location where the clang compilation database json file will be generated. |

