---
title: Define dependency on module(s) in another package
weight: 92
---

This topic describes how to set dependencies on a subset of modules in another package.

<a name="DependencyOnModulesAnotherPackage"></a>
## Usage ##

Package level dependencies imply dependency on ALL [Modules]( {{< ref "reference/packages/modules.md" >}} ) in a particular package.
If the other package contains multiple [Modules]( {{< ref "reference/packages/modules.md" >}} ) you might want to constrain that down to
just one or two of those Modules. In Framework this requires to define

 - Package level dependency
 - List of modules from the dependent package that we want to depend on.




```xml
<property name="runtime.A.builddependencies" value="OtherPackage"/>
<property name="runtime.A.OtherPackage.runtime.buildmodules" value="module1"/>
```
There may be cases when different types of dependencies are needed for different groups of modules from the same dependent package.
To support this functionality several types of properties that can be used to list modules in dependent package can be used.


{{% panel theme = "info" header = "Note:" %}}
In {{< url Framework2 >}} dependencies of all modules in the subset were assuming type of the Package level dependency.

In {{< url Framework3 >}} it is possible to set different types of dependencies for different modules in the dependent package.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
There is a special case in Framework code where if you have a native library with a managed or dotnet module as a dependent it will be converted to a use dependency and as a result will not appear in a generated visualstudio solution.
Declaring an explicit dependency in the way described on this page and override this behavior and ensure the dependent package shows up in the generated solution.
{{% /panel %}}
More formal definition of a module level dependency from another package:


```
[group].[module].[dependency-type] = 'list of package names'

[group].[module].[dependent-package-name].[dependent-group].[dependency-type-for-modules] = 'list of module names from given dependent package'
```
Where

   `[group]`  - is module {{< url buildgroup >}}.
   `[module]`  - name of the module, see also {{< url groupname >}}
   `[dependent-package-name]` - name of the dependent package. This name must be present in one of the package level dependency definitions.
   `[dependent-group]`  - {{< url buildgroup >}}that module names listed in this property belong to.
   `[dependency-type-for-modules]` - one of the following values:

Type |interface |link |build |auto |Framework 2 compatible |
--- |--- |--- |--- |--- |--- |
| usebuildmodules | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |  |
| interfacebuildmodules | ![requirements 1a]( requirements1a.gif ) |  |  |  |  |
| uselinkbuildmodules |  | ![requirements 1a]( requirements1a.gif ) |  |  |  |
|  |
| buildmodules | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |
| linkbuildmodules |  | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildinterfacebuildmodules | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| buildonlybuildmodules |  |  | ![requirements 1a]( requirements1a.gif ) |  |  |
| autobuildmodules | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autointerfacebuildmodules | ![requirements 1a]( requirements1a.gif ) |  | ??? | ![requirements 1a]( requirements1a.gif ) |  |
| autolinkbuildmodules |  | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) |  |


{{% panel theme = "danger" header = "Important:" %}}
Type of the buildmodules property overrides type of the package level dependency definitions.
{{% /panel %}}
## Examples ##

Below is an example of a build file where a package depends on only one of the modules of another package.


```xml
<project default="build">
  <package name="build_dependency_2" targetversion="1.00.00" />

  <property name="runtime.ProgramModule.buildtype" value="Program" />

  <!-- Indicate location of source files -->
  <fileset name="runtime.ProgramModule.sourcefiles" >
    <includes name="${package.dir}/source/*.cpp" />
  </fileset>

  <!-- Declare build dependencies -->
  <property name="runtime.ProgramModule.builddependencies">
    multi_module_library
  </property>
  <property name="runtime.ProgramModule.multi_module_library.runtime.buildmodules">
    module2
  </property>

  <!-- Define what modules to build -->
  <property name="runtime.buildmodules" >
    ProgramModule
  </property>
</project>
```
As a new feature in Framework, a package can depend on a module in a different build group of a package.
In the below example a package depends on two modules, one from the runtime build group and another from the test build group.


```xml
<!-- Declare build dependencies -->
<property name="runtime.ProgramModule.builddependencies">
multi_module_library
</property>
<property name="runtime.ProgramModule.multi_module_library.runtime.buildmodules">
module2
</property>
<property name="runtime.ProgramModule.multi_module_library.test.buildmodules">
testlib1
</property>
```
