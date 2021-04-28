---
title: Usedefaultassemblies
weight: 163
---

This topic describes how tell framework not to add default assemblies.

## Usage ##

##### groupname.csproj.usedefaultassemblies #####


Setting this property to false for a C# or MCPP module will prevent framework from adding default assembly references.

Default value is true

## Example ##


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.          <property name="runtime.MyModule.usedefaultassemblues" value="false" />
```
