---
title: unittest
weight: 162
---

 `unittest` property sets Visual Studio project type to &#39;Unit Test&#39;.

## Usage ##

##### groupname.csproj.unittest  (groupname.fsproj.unittest) #####


Setting this property to true for a C# module will create Unit Test Visual Studio project.

Assembly references added:

   *Microsoft.VisualStudio.QualityTools.UnitTestFramework.dll*

Properties used to define unittest project:

 - {{< url groupname >}} `.csproj.unittest` - for C# modules<br><br>{{< url groupname >}} `.fsproj.unittest` - for F# modules

## Example ##


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.unittest" value="true"/>
```
