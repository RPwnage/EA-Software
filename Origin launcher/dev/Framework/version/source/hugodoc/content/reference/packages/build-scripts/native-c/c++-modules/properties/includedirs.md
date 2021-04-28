---
title: includedirs
weight: 122
---

This element is used to set include directories used to build a module. 

{{% panel theme = "info" header = "Note:" %}}
Includedirs specified in this way are only used while building code for this module. To export include directories to be cosumed by dependent modules see documetnation on [exporting include directories through Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/propertincludedirs.md" >}} ).  
{{% /panel %}}

<a name="IncludedirsUsage"></a>
## Usage ##

Define a property{{< url groupname >}} `.includedirs` in your build script.
Each include directory should be on a separate line.

If `.includedirs` property is not defined framework by default will add following include directories:

 - `${package.dir}/include`
 - `${package.dir}/${eaconfig.${group}.sourcedir}` <br><br>For ${group} values:<br><br>  - *runtime* :  `${package.dir}/source`<br>  - *test* :  `${package.dir}/tests`<br>  - *example* :  `${package.dir}/examples`<br>  - *tool* :  `${package.dir}/tools`

## Example ##

This example demonstrates how to add include directories in structured XML. Conditional inclusion can be handled via `<do>` element.


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <includedirs>
          include
          <!-- example how to add include directories conditionally -->
          <do if="${config-system}==ps3">
            include/ps3
          </do>
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>

```
In traditional syntax include directories can be set through property.


```xml

  <property name="runtime.Hello.includedirs">
    ${package.dir}/include
  </property>

  <property name="runtime.Hello.includedirs" if="${config-system}==ps3">
    ${property.value}
    ${package.dir}/include/ps3
  </property>

```

{{% panel theme = "info" header = "Note:" %}}
In Structured XML relative directories in  `<includedirs>`  element are prepended with package directory automatically.

In traditional syntax full path definition is required.
{{% /panel %}}

##### Related Topics: #####
-  [Exporting include directories through Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/propertincludedirs.md" >}} ) 
