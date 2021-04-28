---
title: targetframeworkversion
weight: 131
---

 `targetframeworkversion` property defines the .Net Framework version for managed C++ modules

<a name="WarningsupressionUsage"></a>
## Usage ##

Define SXML `<config><targetframeworkversion>`  element or a property {{< url groupname >}} `.targetframeworkversion` in your build script.
The version number should be prepended with the letter &#39;v&#39;.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <config>
          <targetframeworkversion>v4.5</targetframeworkversion>
        </config>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>

```

##### Related Topics: #####
-  [targetframeworkversion in .Net (C#) modules]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/targetframeworkversion.md" >}} ) 
