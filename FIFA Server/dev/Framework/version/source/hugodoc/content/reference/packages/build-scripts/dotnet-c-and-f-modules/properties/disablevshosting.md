---
title: disablevshosting
weight: 155
---

 `disablevshosting` property used to enable/disable Visual Studio hosting process during debugging.

## Usage ##

Property used to set Visual Studio Hosting:

{{< url groupname >}} `.csproj.disablevshosting` - for C# modules

Default value is `false` 

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <disablevshosting value="true"/>
      ...
    </CSharpProgram>

```
In traditional syntax, disablevshosting can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.csproj.disablevshosting" value="true"/>
```
