---
title: Warning Level
weight: 87
---

This page describes how to control the warning level used by the build.

<a name="overview"></a>
## Overview ##

Most compilers support a number of different warning levels.
Traditionally framework has turned on all warnings and only given users an option to turn warnings on and off.
Some users would prefer to use a different warning level, often because some of the warnings emitted at the &quot;-Wall&quot; level are not very important or they want to test there code against a higher standard.
Users would have needed to turn off warnings and add the warning level flag they wanted directly to the compiler options, but this was fairly complicated and users need to know what flags each compiler supported.

It is now possible to set the warning level by picking from a pre-defined list of warning levels.
This gives users the ability to globally override the warning level for testing and also allows them to override the warning level for individual modules and buildtypes.

The warning levels that we currently have defined are: &quot;medium&quot;, &quot;high&quot; and &quot;pedantic&quot;. The default setting when warnings are turned on is &quot;high&quot;.

 - `high` - Enables all warnings using the &quot;-Wall&quot; flag.<br>This is the warninglevel that eaconfig has traditionally used as its default.
 - `medium` - Disables some less important warnings, uses the &quot;-W4&quot; flag on MSVC, and on GCC/Clang since &quot;-W4&quot; is not supported it uses no flag.<br>We have initially designed the medium level to match frostbite&#39;s default warning level.
 - `pedantic` - Enables additional diagnostic/langauge warnings.<br>On GCC it uses the flags &quot;-Wall -Wpedantic&quot;.<br>On Clang it uses the flags &quot;-Wall -Weverything -pedantic&quot;.<br>Other compilers, like MSVC, that don&#39;t support these flags will simply use &quot;-Wall&quot;.<br>These settings are not used very often but we have added these as an option for users that want to test their code against the highest standard.

<a name="global"></a>
## Setting the Global warning level ##

The warning level can be changed globally using a global property called &quot;eaconfig.warninglevel&quot;.
This makes it easy to test a build on a higher warning level, for example to give a sense of what warnings need to be fixed before incrementing the warning level more permanently.

###### setting the warninglevel in a masterconfig file's global property section ######

```xml
<globalproperties>
 eaconfig.warninglevel=medium
</globalproperties>
```
<a name="package"></a>
## Setting the warning level for a specific module ##

The warning level of individual modules can be set in structured xml.

###### setting the warninglevel of a module ######

```xml
<Program name="MyCppModule">
 <config>
  <buildoptions>
   <option name="warninglevel" value="medium"/>
  </buildoptions>
 </config>
 <sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cpp" />
 </sourcefiles>
</Program>
```
<a name="package"></a>
## Setting the warning level for a specific package ##

You can also change the warninglevel of custom build types in structured xml.

###### setting the warning level of a build type ######

```xml
<BuildType name="MyCustomProgramBuildType" from="Program">
 <option name="warninglevel" value="medium"/>
</BuildType>
```
