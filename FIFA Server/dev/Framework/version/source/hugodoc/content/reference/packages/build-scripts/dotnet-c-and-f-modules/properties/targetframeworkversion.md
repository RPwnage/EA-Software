---
title: targetframeworkversion
weight: 161
---

 `targetframeworkversion` property defines .Net Framework version.

<a name="WarningsupressionUsage"></a>
## Usage ##

Define SXML `<config><targetframeworkversion>`  element or a property {{< url groupname >}} `.targetframeworkversion` in your build script.
The version number should be prepended with the letter &#39;v&#39;.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <CSharpLibrary name="Hello">
        <config>
          <targetframeworkversion>v4.5</targetframeworkversion>
        </config>
        <sourcefiles>
          <includes name="source/hello/*.cs" />
        </sourcefiles>
      </CSharpLibrary>

```
In traditional syntax:


```xml

    <property name="runtime.Hello.targetframeworkversion" value="v4.5"/>

```

##### Related Topics: #####
-  [targetframeworkversion in Managed C++ modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/targetframeworkversion.md" >}} ) 
