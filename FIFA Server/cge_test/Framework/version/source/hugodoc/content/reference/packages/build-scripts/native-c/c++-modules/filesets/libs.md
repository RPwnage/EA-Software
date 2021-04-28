---
title: libs
weight: 143
---

This topic describes how to add libraries to linker a Native (C/C++) module

## Usage ##

Use `<libraries>`  in SXML or define a {{< url fileset >}} with name {{< url groupname >}} `.libs` in your build script.


{{% panel theme = "info" header = "Note:" %}}
You can add library directories and define library names without path, use &#39;asis&#39; attribute to define name without a path in the fileset.
{{% /panel %}}
## Example ##

Use `<libraries>` element in SXML


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
      <libraries>
        <includes name="lib/boost.lib" />
      </libraries>
    </Program>

```
Example using  [library directories]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/librarydirs.md" >}} ) 


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
      <libdirs>
        lib
      </libdirs>
      <libraries>
        <includes name="boost.lib" asis="true"/>
      </libraries>
    </Program>

```
