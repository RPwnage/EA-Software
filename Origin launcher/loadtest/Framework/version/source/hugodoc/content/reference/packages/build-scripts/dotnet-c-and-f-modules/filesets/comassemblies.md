---
title: comassemblies
weight: 170
---

This topic describes how to add COM assemblies into DotNet module

## Usage ##

Define a{{< url fileset >}} with name {{< url groupname >}} `.comassemblies` in your build script.

Framework will generate managed wrappers and use them in the build or Visual Studio Project generation

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <comassemblies>
        <includes name="${package.dir}\comassemblies\*.dll"/>
      </comassemblies>
      ...
    </CSharpProgram>

```
In traditional syntax, comassemblies fileset can be set as follows:


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <fileset name="runtime.MyModule.comassemblies">
.              <includes name="${package.dir}\comassemblies\*.dll"/>
.          </fileset>
```
