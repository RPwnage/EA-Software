---
title: excludedbuildfiles
weight: 141
---

This topic describes how to add non buildable files to generated  Visual Studio or Xcode project for a Native (C/C++) module

## Usage ##

Use `<excludedbuildfiles>`  in SXML or define  a {{< url fileset >}} with name {{< url groupname >}} `.vcproj.excludedbuildfiles` in your build script.

## Example ##

Use `<excludedbuildfiles>` element in SXML


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
      <visualstudio>
          <excludedbuildfiles>
              <includes name="doc/releasenotes.txt" />
          </excludedbuildfiles>
      </visualstudio>
    </Library>

```
