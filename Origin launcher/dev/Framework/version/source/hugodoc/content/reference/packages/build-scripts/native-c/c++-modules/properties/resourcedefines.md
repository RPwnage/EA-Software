---
title: res.defines
weight: 127
---

This element is used to set additional defines for the resource compiler tool (rc)

<a name="IncludedirsUsage"></a>
## Usage ##

Define `defines`  attribute in  `<resourcefiles`  fileset or a property {{< url groupname >}} `.res.defines` in your build script.

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
           defines="mydefine">
          <includes name="source\hello\*.ico" basedir="${package.dir}/sources/hello"/>
          <includes name="source\hello\*.rc"/>
        </resourcefiles>
    </WindowsProgram>

```
In traditional syntax include directories can be set through property.


```xml

  <property name="runtime.Hello.res.defines">
    mydefine
  </property>


```
