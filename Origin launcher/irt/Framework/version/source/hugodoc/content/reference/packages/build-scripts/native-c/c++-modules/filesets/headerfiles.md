---
title: headerfiles
weight: 142
---

This topic describes how to set headerfiles for a Utility module

## Usage ##

Define a{{< url fileset >}} with name {{< url groupname >}} `.headerfiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
If `headerfiles` fileset is not provided explicitly Framework will add default values
using following patterns:

 - `"${package.dir}/include/**/*.h"`
 - `"${package.dir}/${module.groupsourcedir}/**/*.h"`

Where GroupSourceDir equals to

 - `${eaconfig.${module.buildgroup}.sourcedir"}/${module.name};` - for packages with modules
 - `${eaconfig.${module.buildgroup}.sourcedir"}` -for packages without modules.
{{% /panel %}}
## Example ##


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <Library name="Hello">
      <includedirs>
        include
      </includedirs>
      <headerfiles>
        <includes name="include/hello/**.h" />
        <includes name="source/hello/**.h" />
      </headerfiles>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
      </sourcefiles>
    </Library>

```
