---
title: Pre/Post Build Steps
weight: 148
---

Sections in this topic describe pre and post build steps in DotNet modules, definition options, usage and examples

<a name="Usage"></a>
## Pre/Post build steps ##

Pre and post build steps can be defined in DotNet Modules the same way these steps are defined in Native modules

See [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) for details.

Tee only difference with [Native (C/C++) modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/_index.md" >}} )  and  [Utility modules]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) is that command definition property names use different prefix

 - **csproj.** - for C# modules
 - **fsproj.** - for F# modules

List of command property names for prebuild steps:

   eaconfig.csprog.pre-build-step
   eaconfig.${config-platform}.csproj.pre-build-step
   option csproj.pre-build-step
   {{< url groupname >}}.csproj.pre-build-step

List of command property names for post-build steps:

   eaconfig.csprog.post-build-step
   eaconfig.${config-platform}.csproj.post-build-step
   option csproj.post-build-step
   {{< url groupname >}}.csproj.post-build-step

<a name="Example"></a>
## Example ##


```xml
.      <property name="runtime.buildmodules" value="MyModule" />

.      <property name="runtime.MyModule.buildtype" value="CSharpProgram"/>
.      <property name="runtime.MyModule.csproj.pre-build-step">
.        @echo Executing runtime.MyModule.csproj.pre-build-step
.      </property>
.      <property name="runtime.MyModule.csproj.post-build-step">
.        @echo Executing runtime.MyModule.csproj.post-build-step
.      </property>
```

##### Related Topics: #####
-  [Pre/Post Build Steps in Native modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) 
