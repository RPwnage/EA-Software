---
title: excludedbuildfiles
weight: 172
---

This topic describes how to set additional files that do not participate in build but added to the Visual Studio project as non buildable files.

## Usage ##

Define following fileset.

 - {{< url groupname >}} `.csproj.excludedbuildfiles` - for C# modules<br><br>{{< url groupname >}} `.fsproj.excludedbuildfiles` - for F# modules

## Example ##


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <fileset name="runtime.MyModule.excludedbuildfiles">
.              <includes name="${package.dir}\text\**"/>
.          </fileset>
```
