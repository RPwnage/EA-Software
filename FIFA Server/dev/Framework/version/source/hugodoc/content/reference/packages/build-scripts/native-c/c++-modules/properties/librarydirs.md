---
title: libdirs
weight: 123
---

This element is used to set library directories used to build a module.

<a name="LibrarydirsUsage"></a>
## Usage ##

SXML `<libdirs>`  element or a property {{< url groupname >}} `.libdirs` in your build script.
Each library directory should be on a separate line.

## Example ##

This example demonstrates how to add library directories in structured XML. Conditional inclusion can be handled via `<do>` element.


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <Library name="Hello">
      <includedirs>
        include
      </includedirs>
      <libdirs>
        libs
      </libdirs>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
      </sourcefiles>
    </Library>

```
In traditional syntax include directories can be set through property.


```xml

  <property name="runtime.Hello.libdirs">
    ${package.dir}/lib
  </property>

  <property name="runtime.Hello.libdirs" if="${config-system}==ps3">
    ${property.value}
    ${package.dir}/lib/ps3
  </property>

```

{{% panel theme = "info" header = "Note:" %}}
In Structured XML relative directories in `<libdirs>` element are prepended with package directory automatically.

In traditional syntax full path definition is required.
{{% /panel %}}

##### Related Topics: #####
-  [Exporting library directories through Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/propertylibdirs.md" >}} ) 
