---
title: sourcefiles
weight: 146
---

This topic describes how to set sourcefiles for a Native (C/C++) module

## Usage ##

Use `<sourcefiles>`  in SXML or define  a {{< url fileset >}} with name {{< url groupname >}} `.sourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Subsets of sourcefiles can be assigned different buildtype{{< url optionset >}}.

For example, C sourcefiles should be assigned C options
(standard option set with C options is `'LibraryC'`
{{% /panel %}}
Framework can combine source files into [Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) compilation units. See [Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) topic for details how to
configure bulk build.

## Example ##

Use `<sourcefiles>` element in SXML


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <Library name="Hello">
      <includedirs>
        include
      </includedirs>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
      </sourcefiles>
    </Library>

```
Setting different compiler options for subsets of source files. Objective C files require different compiler options:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <Library name="Hello">
      <includedirs>
        include
      </includedirs>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
        <includes name="${package.dir}/source/Apple/**.mm" optionset="ObjectiveCPPLibrary"
            if="${config-system} == iphone || ${config-system} == iphone-sim || ${config-system} == osx" />
      </sourcefiles>
    </Library>

```
