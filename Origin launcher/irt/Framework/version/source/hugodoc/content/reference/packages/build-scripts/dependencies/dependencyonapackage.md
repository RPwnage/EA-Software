---
title: Define dependency on another package
weight: 91
---

This topic describes how to set dependencies on another package

<a name="DependencyOnAnotherPackage"></a>
## Usage ##

Package level dependency definition creates dependency on all modules
in `runtime` {{< url buildgroup >}}of dependent package.

This is the simplest and most widely used definition:

###### build dependency of module B on EAThread package ######

```xml
<property name="runtime.B.builddependencies" value="EAThread"/>
```
More formal definition of a package level dependency:


```
[group].[module].[dependency-type] = 'list of package names'
```
Where

   `[group]`  - is module {{< url buildgroup >}}.
   `[module]`  - name of the module, see also {{< url groupname >}}.
   `[dependency-type]` - one of the following values:

Type |interface |link |build |auto |Framework 2 compatible |
--- |--- |--- |--- |--- |--- |
| usedependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  | ![requirements 1a]( requirements1a.gif ) |
| interfacedependencies | ![requirements 1a]( requirements1a.gif ) |  |  |  | ![requirements 1a]( requirements1a.gif ) |
| uselinkdependencies |  | ![requirements 1a]( requirements1a.gif ) |  |  |  |
|  |
| builddependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |
| linkdependencies |  | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildinterfacedependencies | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildonlydependencies |  |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| autodependencies | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autointerfacedependencies | ![requirements 1a]( requirements1a.gif ) |  | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autolinkdependencies |  | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |


{{% panel theme = "primary" header = "Tip:" %}}
Package dependency always create dependency on the runtime {{< url buildgroup >}} of dependent package.

To declare dependency on the modules in other than runtime group use [dependency on particular module(s) in another package]( {{< ref "reference/packages/build-scripts/dependencies/dependencyonamoduleinanotherpackage.md" >}} ) .
{{% /panel %}}
## Examples ##

Below is the *.build file of a package called build_dependency whose module called ProgramModule depends on another package called simple_library.


```xml
<project default="build">
  <package name="build_dependency" />

  <property name="runtime.ProgramModule.buildtype" value="Program" />

  <!-- Indicate location of source files -->
  <fileset name="runtime.ProgramModule.sourcefiles" >
    <includes name="${package.dir}/source/*.cpp" />
  </fileset>

  <!-- Declare build dependencies -->
  <property name="runtime.ProgramModule.builddependencies">
    simple_library
  </property>

  <!-- Define what modules to build -->
  <property name="runtime.buildmodules" >
    ProgramModule
  </property>
</project>
```
