---
title: usingdirs
weight: 132
---

This element is used to set &#39;using&#39; directories used to build a module.

<a name="UsingdirsUsage"></a>
## Usage ##

Use `<usingdirs>`  in SXML or define a property with the name {{< url groupname >}} `.usingdirs` in your build script.
This will set VC compiler */AI* directories. Each directory should be on a separate line.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Program name="Hello">
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <usingdirs>
          ${package.dir}/asemblies
        </usingdirs>
    </Program>

```

##### Related Topics: #####
-  [Exporting using directories through Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/propertyusingdirs.md" >}} ) 
