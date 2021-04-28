---
title: rootnamespace
weight: 130
---

property sets rootnamespace value for managed resource compiler

<a name="rootnamespaceUsage"></a>
## Usage ##

Define SXML `<config><rootnamespace>`  element or a property {{< url groupname >}} `.rootnamespace` in your build script.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <ManagedCPPLibrary name="Hello">
        <rootnamespace>EA.Hello</rootnamespace>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <resourcefiles>
           . . .
        </resourcefiles>
      </ManagedCPPLibrary>

```

##### Related Topics: #####
-  [targetframeworkversion in .Net (C#) modules]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/targetframeworkversion.md" >}} ) 
