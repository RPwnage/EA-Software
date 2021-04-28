---
title: pch-file
weight: 124
---

To define path for the precompiled header file use property  *pch-file* 

<a name="PchFileUsage"></a>
## Usage ##

Define SXML element `<config><pch pchfile=""`  or a property {{< url groupname >}} `.pch-file` in your build script.

The value should contain full path for the pch file

If `.pch-file`  property is not defined framework by default will use value  *&quot;%intermediatedir%\stdPCH.pch&quot;* .


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
          <pch enable="true" pchfile="%intermediatedir%\precompiled.pch"/>
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

  <property name="runtime.Hello.pch-file" value="%intermediatedir%\precompiled.pch"/>

```
