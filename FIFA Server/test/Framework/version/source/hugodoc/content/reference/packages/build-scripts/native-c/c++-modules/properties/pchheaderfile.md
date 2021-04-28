---
title: pch-header-file
weight: 125
---

This element is used to set the name of precompiled header file.

<a name="PchHeaderFileUsage"></a>
## Usage ##

Define SXML element `<config><pch pchheader=""`  or a property {{< url groupname >}} `.pch-header-file` in your build script.

The value should contain name of the file without path

If `.pch-header-file`  property is not defined framework by default will use value  *&quot;stdPCH.h&quot;* .


{{% panel theme = "info" header = "Note:" %}}
In order to properly use this feature, you still need to mark a source file with &#39;create-precompiled-header&#39; optionset as shown in following example.
{{% /panel %}}
## Example ##

This example demonstrates how to add include directories in structured XML. Conditional inclusion can be handled via `<do>` element.


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <config>
          <pch enable="true" pchheader="pch.h"/>
        </config>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/pch.cpp" optionset="create-precompiled-header"/>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>

```
In traditional syntax set property.


```xml

  <property name="runtime.Hello.pch-header-file" value="pch.h"/>

```
