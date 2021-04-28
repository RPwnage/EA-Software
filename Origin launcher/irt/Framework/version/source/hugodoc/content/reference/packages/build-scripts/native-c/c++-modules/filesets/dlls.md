---
title: dlls
weight: 140
---

This topic describes how to add additional dll files for a Native (C/C++) module.

## Usage ##

Sometimes there is a need to pass dynamic library files to a linker.

Use `<dlls>`  in SXML or define {{< url fileset >}} with name {{< url groupname >}} `.dlls` in your build script.

Files in the `dlls` fileset are added to the linker input.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Program name="Hello">
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <dlls if="${config-compiler}==vc">
          <includes name="bin/**.dll"/>
        </dlls>
    </Program>

```
