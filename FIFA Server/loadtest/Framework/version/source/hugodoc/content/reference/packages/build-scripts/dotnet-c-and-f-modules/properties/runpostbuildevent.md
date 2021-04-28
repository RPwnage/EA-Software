---
title: runpostbuildevent
weight: 160
---

 `runpostbuildevent` property lets to set condition when to run a postbuild event in Visual Studio.

## Usage ##

Properties used to define runpostbuildevent condition:

 - {{< url groupname >}} `.csproj.runpostbuildevent` - for C# modules<br><br>{{< url groupname >}} `.fsproj.runpostbuildevent` - for F# modules

This property can take on of the following values.

 - **OnOutputUpdated**
 - **Always**
 - **OnSuccessfulBuild**

Default value is **OnOutputUpdated** 

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <runpostbuildevent value="Always"/>
      ...
    </CSharpProgram>

```
In traditional syntax, runpostbuildevent can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.csproj.runpostbuildevent" value="Always"/>
```
