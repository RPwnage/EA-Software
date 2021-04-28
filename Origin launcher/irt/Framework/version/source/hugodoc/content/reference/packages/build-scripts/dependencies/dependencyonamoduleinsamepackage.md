---
title: Define dependency on a module(s) in the same package
weight: 93
---

This topic describes how to set dependencies on a module in the same package

<a name="DependencyOnAModuleInSamePackage"></a>
## Usage ##

Dependencies between modules in the same package (in Framework 2 called moduledependencies) are defined differently than dependencies on modules in another package.

###### build dependency of module B on modules C and D from runtime group of the same package ######

```xml
<property name="runtime.B.runtime.moduledependencies" value="C D"/>
```

{{% panel theme = "info" header = "Note:" %}}
Build script setup in Structured XML format is actually much simpler.  See below example.
{{% /panel %}}
More formal definition of a module level dependency:


```
[group].[module].[dependent-group].[dependency-type] = 'list of module names'
```
Where

   `[group]`  - is module {{< url buildgroup >}}.
   `[module]` - name of the module.
   `[dependent-group]`  - is dependent module {{< url buildgroup >}}.
   `[dependency-type]` - one of the following values:

Type |interface |link |build |auto |Framework 2 compatible |
--- |--- |--- |--- |--- |--- |
| usemoduledependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |  |
| interfacemoduledependencies | ![requirements 1a]( requirements1a.gif ) |  |  |  |  |
| uselinkmoduledependencies |  | ![requirements 1a]( requirements1a.gif ) |  |  |  |
|  |
| moduledependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |
| linkmoduledependencies |  | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildinterfacemoduledependencies | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildonlymoduledependencies |  |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| automoduledependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autointerfacemoduledependencies | ![requirements 1a]( requirements1a.gif ) |  | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autolinkmoduledependencies |  | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |


{{% panel theme = "primary" header = "Tip:" %}}
When building modules in `test` ,  `example` groups Framework automatically adds &#39;automoduledependencies&#39; on all modules in the runtime group,
unless there is explicit dependency definition on a module(s) in `runtime` group.

To remove automatic dependency on the runtime group add any module level dependency to the module. This module level dependency property can be set to empty string if no dependencies are needed.

Another way to skip automatic addition of dependencies on runtime modules is to define properties with value `true` :

{{< url groupname >}} `.skip-runtime-dependency` - skip automatic addition of runtime dependendencies for given module.

{{< url buildgroup >}} `.skip-runtime-dependency` - skip automatic addition of runtime dependendencies for all modules in given group.

Or in structured XML using &lt;dependencies skip-runtime-dependency=&quot;true&quot;/&gt;
{{% /panel %}}
## Examples ##

Below is an example of a build file where one module in a package depends on other modules in the same package.

###### In Structured XML Syntax ######

```xml
<project default="build">
  <package name="mypackagename" />

  <Library name="module1">
    <includedirs>
      ${package.dir}/include
    </includedirs>
    <sourcefiles>
      <includes name="${package.dir}/source/module1/*.cpp"/>
    </sourcefiles>
  </Library>

  <Library name="module2">
    <includedirs>
      ${package.dir}/include
    </includedirs>
    <sourcefiles>
      <includes name="${package.dir}/source/module2/*.cpp"/>
    </sourcefiles>
  </Library>

  <Program name="ProgramModule">
    <dependencies>
      <auto>
        <!-- Depends on 'module1' module of current package 'mypackagename' -->
        mypackagename/module1
      </auto>
    </dependencies>
    <includedirs>
      ${package.dir}/include
    </includedirs>
    <sourcefiles>
      <includes name="${package.dir}/source/program/*.cpp"/>
    </sourcefiles>
  </Library>
</project>
```
###### Old Style Build Script ######

```xml
<project default="build">
  <package name="mypackagename" />

  <property name="runtime.module2.buildtype" value="Library"/>
  <property name="runtime.module1.buildtype" value="Library"/>
  <property name="runtime.ProgramModule.buildtype" value="Program" />

  <!-- Include directories of library modules -->
  <property name="runtime.module1.includedirs" >
    ${package.dir}/include
  </property>

  <property name="runtime.module2.includedirs" >
    ${package.dir}/include
  </property>

  <!-- Indicate location of source files -->
  <fileset name="runtime.module1.sourcefiles" >
    <includes name="${package.dir}/source/module1/*.cpp" />
  </fileset>

  <fileset name="runtime.module2.sourcefiles" >
    <includes name="${package.dir}/source/module2/*.cpp" />
  </fileset>
  
  <fileset name="runtime.ProgramModule.sourcefiles" >
    <includes name="${package.dir}/source/program/*.cpp" />
  </fileset>

  <!-- Define what modules to build -->
  <property name="runtime.buildmodules" >
    module1
    module2
    ProgramModule
  </property>

  <!-- Declare build dependencies -->
  <property name="runtime.ProgramModule.runtime.moduledependencies" >
    module1
  </property>
</project>
```
