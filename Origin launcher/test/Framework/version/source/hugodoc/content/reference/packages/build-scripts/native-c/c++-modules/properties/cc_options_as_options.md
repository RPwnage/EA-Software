---
title: cc.options and as.options
weight: 134
---

Additional compiler options can be added without modifying buildtype

<a name="CcOptionsUsage"></a>
## Usage ##

Define property{{< url groupname >}} `cc.options`  or {{< url groupname >}} `as.options` to add additional compiler options.

It is recommended to define each option on a separate line.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <config>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>
                
      <property name="runtime.Hello.cc.options" value="-showIncludes" if="${config-compiler}==vc"/>

```
There is no SXML field for this property. Use `<config><options></options></config>` SXML element.


##### Related Topics: #####
-  [options can also be set through custom buildoptionset]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 
-  [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 
