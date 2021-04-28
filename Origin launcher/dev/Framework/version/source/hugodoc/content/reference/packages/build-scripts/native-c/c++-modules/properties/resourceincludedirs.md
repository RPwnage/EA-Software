---
title: res.includedirs
weight: 126
---

This element is used to set include directories for resource compiler tool (rc)

<a name="IncludedirsUsage"></a>
## Usage ##

Define `includedirs`  attribute in  `<resourcefiles`  fileset or a property {{< url groupname >}} `.res.includedirs` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Visual Studio and Windows SDK include directories are added by default
{{% /panel %}}
## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <WindowsProgram name="Hello">
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <resourcefiles basedir="${package.dir}/sources"
           includedirs=" ${package.dir}/include">
          <includes name="source\hello\*.ico" basedir="${package.dir}/sources/hello"/>
          <includes name="source\hello\*.rc"/>
        </resourcefiles>
    </WindowsProgram>

```
In traditional syntax include directories can be set through property.


```xml

  <property name="runtime.Hello.res.includedirs">
    ${package.dir}/include
  </property>


```
