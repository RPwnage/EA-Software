---
title: objectfiles
weight: 144
---

This topic describes how to add additional object files for a Native (C/C++) module.

## Usage ##

Sometimes there is a need to add prebuilt object files to a library, program, or dll module.

Use `<objectfiles>`  in SXML or define {{< url fileset >}} with name {{< url groupname >}} `.objectfiles` in your build script.

Files in the  `objectfiles` fileset are added to the librarian or linker input in addition to the object files produced by compiler.

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
        <objectfiles>
          <includes name="bin/**.obj" if="${config-compiler}==vc"/>
          <includes name="bin/**.o" if="${config-compiler}==gcc || ${config-compiler}==clang"/>
        </objectfiles>
    </Program>

```
