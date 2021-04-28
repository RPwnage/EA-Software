---
title: defines
weight: 119
---

This element is used to set preprocessor defines used to build a module.

<a name="DefinesUsage"></a>
## Usage ##

SXML `<config><defines>`  element or a property {{< url groupname >}} `.defines` in your build script.
The value of this property should contain preprocessor definition. Each definition should be on a separate line.


{{% panel theme = "info" header = "Note:" %}}
Preprocessor definitions set through this property will be used to build the module and are not propagated to other modules

For information on scope of defines see  [Defines in Framework]( {{< ref "user-guides/everythingaboutdefines.md" >}} )
{{% /panel %}}
## Example ##

This example demonstrates how to add preprocessor definitions in structured XML. Conditional inclusion can be handled via `<do>` element.


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <config>
          <defines>
            EXAMPLE_DEFINE
            DEFINE_WITH_VALUE='\"My Define Value\"'
            <do if="${config-system}==pc">
              MY_PC_DEFINE
            </do>
          </defines>
        </config>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>

```
In traditional syntax defines can be set through property.


```xml

  <property name="runtime.Hello.defines">
    EXAMPLE_DEFINE
    DEFINE_WITH_VALUE='\"My Define Value\"'
  </property>

  <property name="runtime.Hello.defines" if="${config-system}==pc">
    ${property.value}
    MY_PC_DEFINE
  </property>

```

##### Related Topics: #####
-  [Defines in Framework]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) 
-  [Defines in Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/propertydefines.md" >}} ) 
