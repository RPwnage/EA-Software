---
title: How to debug Structured XML
weight: 205
---

This topic describes debugging facilities in the Structured XML

<a name="DebuggingSXML"></a>
## Debugging Structured XML ##

In addition to  [regular Framework debugging]( {{< ref "user-guides/debugframework.md" >}} ) Structured XML provides ability to insert `<echo>` statements.


{{% panel theme = "info" header = "Note:" %}}
Use [Intellisense]( {{< ref "framework-101/intellisense.md" >}} )  or  [Structured XML Tasks]( {{< ref "reference/tasks/structured-xml-tasks/_index.md" >}} ) to find where `<echo>` can be used in Structured XML.
Usually it is in the same places where the `<do>` element can be used.
{{% /panel %}}

```xml

  <Library name="MyLibrary">
    <do if="${Dll}">
        <buildtype name="DynamicLibrary"/>
        <echo>Setting buildtype 'DynamicLibrary'</echo>
    </do>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>

```
All structured XML modules have  `debug`  attribute. If this attribute is set to true information about framework properties and filesets set by structured XML code is printed to the log file


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="simple_library" />

    <Library name="ExampleLibrary" debug="true" >
      <includedirs>
        ${package.dir}/include
      </includedirs>
      <sourcefiles>
        <includes name="${package.dir}/source/*.cpp" />
      </sourcefiles>
    </Library>

  </project>

```
When  `debug` attribute is not set build output will contain no additional info:


```
[package] simple_library-1.00.00 (pc-vc-dev-debug)  build
[create-build-graph] runtime [Packages 4, modules 1, active modules 1]  (188 millisec)
[nant-build] building simple_library-1.00.00 (pc-vc-dev-debug) 'runtime.ExampleLibrary'
[cc] source\say_hello.cpp
[lib] ExampleLibrary
[nant-build] finished target 'build' (1 modules) (309 millisec)
[nant] BUILD SUCCEEDED (00:00:01)
```
When `debug`  attribute is set to  ` *true* ` following information is printed:


```

[package] simple_library-1.00.00 (pc-vc-dev-debug)  build

[Library] --- Module name="ExampleLibrary"

<property name="runtime.buildmodules" value="ExampleLibrary"/>
<property name="runtime.ExampleLibrary.scriptfile" value="D:\test\simple_library\1.00.00\simple_library.build"/>
<property name="runtime.ExampleLibrary.buildtype" value="Library"/>
<property name="runtime.ExampleLibrary.defines" value=""/>
<property name="runtime.ExampleLibrary.remove.defines" value=""/>
<property name="runtime.ExampleLibrary.includedirs" value="
 D:\test\simple_library\1.00.00/include
 "/>

<fileset name="runtime.ExampleLibrary.sourcefiles" basedir="D:\test\simple_library\1.00.00">
  <includes  name="D:\test\simple_library\1.00.00/source/*.cpp"/>
</fileset>
<property name="runtime.ExampleLibrary.custom-build-before-targets" value=""/>
<property name="runtime.ExampleLibrary.custom-build-after-targets" value=""/>
<property name="runtime.ExampleLibrary.copylocal" value="False"/>
<optionset name="runtime.ExampleLibrary.webreferences" />
<property name="runtime.ExampleLibrary.workingdir" value=""/>
<property name="runtime.ExampleLibrary.commandargs" value=""/>
<property name="runtime.ExampleLibrary.commandprogram" value=""/>
<property name="runtime.ExampleLibrary.run.workingdir" value=""/>
<property name="runtime.ExampleLibrary.run.args" value=""/>
<property name="runtime.ExampleLibrary.use.pch" value="false"/>


--- Build Options ---

<optionset name="Library" />

[Library] --- End Module "ExampleLibrary"

[create-build-graph] runtime [Packages 4, modules 1, active modules 1]  (188 millisec)
[nant-build] building simple_library-1.00.00 (pc-vc-dev-debug) 'runtime.ExampleLibrary'
[cc] source\say_hello.cpp
[lib] ExampleLibrary
[nant-build] finished target 'build' (1 modules) (322 millisec)
[nant] BUILD SUCCEEDED (00:00:01)

```

##### Related Topics: #####
-  [Debugging Framework]( {{< ref "user-guides/debugframework.md" >}} ) 
