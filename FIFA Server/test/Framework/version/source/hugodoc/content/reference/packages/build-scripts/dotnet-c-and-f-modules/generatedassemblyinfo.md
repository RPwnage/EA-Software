---
title: Generated Assembly Info Files
weight: 180
---

This topic describes how you can generated assemblyinfo files for C#, F# and C++/CLI modules

<a name="overview"></a>
## Overview ##

An assemblyinfo file can be added to a dotnet module in order to define assembly metadata.
Users can add their own assemblyinfo file to a module&#39;s sourcefiles fileset.
They can also use the assemblyinfo element in structured xml and framework will generate an assembly info file for them.
Assemblyinfo files are supported for C#, F# and C++/CLI modules.

<a name="examples"></a>
## Example ##

A default assemblyinfo file will be generated and added to the project if the assemblyinfo structured xml element is added to the module definition.


```xml

<CSharpProgram name="mymodule">
 <assemblyinfo/>
 <sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cs" />
 </sourcefiles>
</CSharpProgram>
         
```
The fields in the assemblyinfo file can be overriden on the structured xml task.


```xml

<CSharpProgram name="mymodule">
 <assemblyinfo company="customcompany" copyright="customcopyright" version="9.9.9.9"
   title="customtitle" product="customproduct" description="customdescription"/>
 <sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cs" />
 </sourcefiles>
</CSharpProgram>
         
```
