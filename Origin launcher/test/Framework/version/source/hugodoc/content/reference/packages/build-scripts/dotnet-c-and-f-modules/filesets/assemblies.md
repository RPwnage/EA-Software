---
title: assemblies
weight: 168
---

This topic describes how to set referenced assemblies for a DotNet module

## Usage ##

Define SXML element `<assemblies>`  or a {{< url fileset >}} with name {{< url groupname >}} `.assemblies` in your build script.

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <assemblies>
        <includes name="${package.dir}\assemblies\**.dll">
      </assemblies>
      ...
    </CSharpProgram>

```
In traditional syntax, assemblies fileset can be set as follows:


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <fileset name="runtime.MyModule.assemblies">
.              <includes name="${package.dir}\assemblies\**.dll"/>
.          </fileset>
```
