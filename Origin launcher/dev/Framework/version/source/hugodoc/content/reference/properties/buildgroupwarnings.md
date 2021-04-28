---
title: Build Group Warnings
weight: 231
---

If a package has modules in different build groups it may become confusing to users if they try to build the modules using the wrong groups.
This topic decribes a property that can be set to warn or fail if a user tries to build a package or module using the wrong group type.

<a name="Section1"></a>

<a name="WarnAndErrorMessages"></a>
### Set Warning or Error when build targets for a group are executed ###

You can specify properties to trigger warning or error when build or run target for a given{{< url buildgroup >}}is executed

 - {{< url groupname >}} `.warn.message` - When build or run target is executed for given group and module the message will be printed.
 - `package.` {{< url buildgroup >}} `.warn.message` - When build or run target is executed for given group the message will be printed.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>If module specific &#39;.warn.message&#39; property exists it will be used instead.<br>{{% /panel %}}
 - {{< url groupname >}} `.error.message` - When build or run target is executed for given group and module the error with given message will be raised.
 - `package.` {{< url buildgroup >}} `.error.message` - When build or run target is executed for given group the error with given message will be raised.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>If module specific &#39;.error.message&#39; property exists it will be used instead.<br>{{% /panel %}}

Example


```xml

    <project xmlns="schemas/ea/framework3.xsd">
  
      <package name="simple_program" />

      <Program name="simple_program">
        <includedirs>
          ${package.dir}/include
        </includedirs>
        <sourcefiles>
          <includes name="source/**.cpp"/>
        </sourcefiles>
      </Program>

      <property name="package.runtime.warn.message" value ="package.runtime.warn.message is DEFINED"/>

      <property name="runtime.simple_program.warn.message" value ="runtime.simple_program.warn.message is DEFINED"/>

      <property name="package.runtime.error.message" value ="package.runtime.error.message is DEFINED"/>

      <property name="runtime.simple_program.error.message" value ="runtime.simple_program.error.message is DEFINED"/>

  </project>

```
Output


```
C:\test\simple_program\1.00.00>D:\packages\Framework\dev\bin\nant.exe build
[package] simple_program-1.00.00 (android-gcc-dev-debug)  build
[warning] runtime.simple_program.warn.message is DEFINED
[nant] BUILD FAILED (00:00:00)

[error] error at task "load-package"  android-gcc-dev-debug
[error]   runtime.simple_program.error.message is DEFINED

at task     "load-package" D:\packages\eaconfig\dev\config\targets\target-build.xml(69,6)
at target   "eaconfig-build-graph" D:\packages\eaconfig\dev\config\targets\target-build.xml(15,8)
at target   "build" D:\packages\eaconfig\dev\config\targets\target-build.xml(13,6)
at project   D:\test\simple_program\1.00.00\simple_program.build
```
