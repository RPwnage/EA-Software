---
title: sourcefiles
weight: 174
---

This topic describes how to set sourcefiles for a DotNet module

## Usage ##

Define SXML `<sourcefiles>`  element or a {{< url fileset >}} with name {{< url groupname >}} `.sourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Any{{< url optionset >}}assigned to files is ignored. Unlike native modules C# modules
do not allow to set custom options on subsets of source files.
{{% /panel %}}
## Example ##


```xml

    <CSharpProgram name="MyModule">
      <sourcefiles>
        <includes name="${package.dir}\src\**.cs">
      </sourcefiles>
      ...
    </CSharpProgram>

```
In traditional syntax, sourcefiles fileset can be set as follows:


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <fileset name="runtime.MyModule.sourcefiles">
.              <includes name="${package.dir}\src\**.cs"/>
.          </fileset>
```
