---
title: asmsourcefiles
weight: 136
---

This topic describes how to set assembly source files for a Native (C/C++) module

## Usage ##

Use `<asmsourcefiles>`  in SXML or define a {{< url fileset >}} with name {{< url groupname >}} `.asmsourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Subsets of assembly sourcefiles can be assigned different buildtype{{< url optionset >}}.
{{% /panel %}}
## Example ##

Use  `<asmsourcefiles>`  element in SXML


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <Library name="Hello">
      <includedirs>
        include
      </includedirs>
      <asmsourcefiles>
        <includes name="source/hello/{config-system}/*.asm"/>
        <includes name="source/hello/{config-system}/*.s" if="${config-system}==unix"/>
      </asmsourcefiles>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
      </sourcefiles>
    </Library>

```
