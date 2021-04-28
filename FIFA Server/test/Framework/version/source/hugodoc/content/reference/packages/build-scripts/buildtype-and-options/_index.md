---
title: buildtype and options
weight: 85
---

The Buildtype value is the name of optionset. This optionset is used to determine what kind of module is it:
&#39;Library&#39;, &#39;Program&#39;, &#39;Utility&#39;, &#39;MakeStyle&#39;, etc. The Buildtype optionset also contains number of flags,
compiler and linker options, and other data that are used during the build.

<a name="buildtype"></a>
## Module Buildtype ##

Buildtype optionsets are special optionsets that must contain certain required options.Depending on the type of module (native, C#, Utility, etc)
these options can include compiler, linker, or librarian options. Buildtype optionsets often include so called flags (like *exceptions=&quot;on/off&quot;* ), these
flags are converted to appropriate platform specific options during framework execution.

Build type can be set by choosing appropriate [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} ) task:

 - Choosing appropriate [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} ) task:<br><br><br>```xml<br><br>    <project default="build" xmlns="schemas/ea/framework3.xsd"><br><br>      <package  name="Example"/><br><br>      <Library name="ExampleLibrary"><br>        . . . .<br>      </Library><br><br>```
 - Specifying buildtype explicitly:<br><br><br>```xml<br><br>    <project default="build" xmlns="schemas/ea/framework3.xsd"><br><br>      <package  name="Example"/><br><br>      <Library name="ExampleLibrary" buildtype="CLibrary"><br>      . . .<br><br>```<br>Using conditions to set a build type:<br><br><br>```xml<br><br>    <project default="build" xmlns="schemas/ea/framework3.xsd"><br><br>      <package  name="Example"/><br><br>      <Library name="ExampleLibrary"><br>       <buildtype name="DynamicLibrary" if="${Dll??false}"/><br>       . . .<br><br>```

In traditional Framework syntax (not Structured XML) build type is defined through property


```xml

<project default="build" xmlns="schemas/ea/framework3.xsd">

  <package  name="Example"/>
            
  <property name="runtime.ExampleLibrary.buildtype" value="Library"/>
            
  <fileset name="runtime.ExampleLibrary.sourcefiles">
     <include name="sources/**.cpp"/>
  </fileset>
  . . . .

```
### For each platform, the eaconfig package creates a number of standard buildtype optionsets. ###

For *native* (C/C++) modules following standard buildtypes and defined in the eaconfig package

 - **Program** use this optionset to build an executable.
 - **Library** use this optionset to define a static C++ library.
 - **CLibrary** use this optionset to define a static C library or to assign the correct settings to C files that are part of a Library module.
 - **DynamicLibrary** use this optionset to build a DLL (shared library on Linux/Unix systems).
 - **CDynamicLibrary** use this optionset to define a shared C library or to assign the correct settings to C files that are part of a DynamicLibrary module.
 - **DynamicLibraryStaticLinkage** This optionset is for dynamic library module that links to static library versions of other modules.<br>The only difference from *DynamicLibrary*  option set is that  `EA_DLL` define is not set, which affects how functions are exported from dependent packages.
 - **ManagedCppProgram** use this optionset to create MCPP program (This is available for PC/PC64 configurations).
 - **ManagedCppAssembly** use this optionset to create MCPP assembly (This is available for PC/PC64 configurations).
 - **WindowsProgram** use this optionset to create Windows program. This optionset adds extra define `_WINDOWS` compared to *Program*  optionset and sets linker option  * `-subsystem:windows` *  (controlled by a flag  * `pcconsole="on/off"` * <br><br>(This is available for PC/PC64 configurations).
 - **ManagedCppWindowsProgram** use this optionset to create MCPP Windows program. This optionset adds  and sets linker option * `-subsystem:windows` *  (controlled by a flag  * `pcconsole="on/off"` * compared to *ManagedCPPProgram* optionset.<br><br>(This is available for PC/PC64 configurations).
 - **WinRTCppProgram, WinRTCppLibrary** Use these optionsets to create WinRT (windows runtime) application or library.
 - **WinRTRuntimeComponent** Use these optionsets to create WinRT (windows runtime) component (dynamic library).

For *.Net* (C#/F#) modules following standard buildtypes and defined in the eaconfig package

 - **CSharpProgram / FSharpProgram** use this optionset to create C# or F# console executable.
 - **CSharpLibrary / FSharpLibrary** use this optionset to create C# or F# assembly.
 - **CSharpWindowsProgram / FSharpWindowsProgram** use this optionset to create C# or F# Windows program. The difference between CSharpProgram and CHarpWindowsProgram is that the latter sets csc target to * `winexe` * , while former has csc target set to  * `exe` *
 - **CSharpWinRTComponent** Use this optionset to create WinRT (windows runtime) C# component

### It is also possible to create custom build type optionsets. ###



 - [Build Setting Properties]( {{< ref "reference/properties/buildsettingproperties.md" >}} ) - this section describes how to change default settings in eaconfig buildtype optionsets using properties.
 - [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) - this section describes how to create custom build types in the package build script.

