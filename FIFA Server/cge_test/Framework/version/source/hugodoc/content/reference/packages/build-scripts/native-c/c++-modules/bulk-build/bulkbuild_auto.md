---
title: Auto Bulk Build
weight: 109
---

Input source files can be automatically converted to bulkbuild source files.

They are two mutually exclusive ways to auto-generate bulkbuild files:

 - Use property bulkbuild.filesets to generate multiple bulk build files.
 - Use fileset runtime.bulkbuild.sourcefiles to generate single bulkbuild file.

<a name="BuilkBuild_Auto_SourceFiles"></a>
## Bulk Build Sourcefiles ##

This is the simple case where the list of loose source files that will form bulkbuild file is specified explicitly
using `*.bulkbuild.sourcefiles` fileset.


{{% panel theme = "danger" header = "Important:" %}}
The normal sourcefiles definition is completely ignored in this case unless [partial builkbuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/builkbuild_partial.md" >}} ) is specified.
{{% /panel %}}
###### Simple case where the user wants to auto-bulkbuild the runtime.sourcefiles in a single 'bulk unit' ######

```xml

  <Library name="Simple">
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
    <!--
    All source files will be included in the bulkbuild.
    Max size of each generated BB file will not exceed 3 includes
    -->
    <bulkbuild enable="true" maxsize="3"/>
  </Library>


```
Same definition in traditional syntax.

###### Simple case where the user wants to auto-bulkbuild the runtime.sourcefiles in a single 'bulk unit' ######

```xml
<!-- A package that doesn’t contain modules. -->
<fileset name="runtime.bulkbuild.sourcefiles" fromfileset="runtime.sourcefiles"/>

<!-- A package that contains the ea_aptaux_core module. -->
<fileset name="runtime.ea_aptaux_core.bulkbuild.sourcefiles" fromfileset="runtime.ea_aptaux_core.sourcefiles"/>
```
<a name="BuilkBuild_Auto_SourceFileSets"></a>
## Multiple Bulk Build Source filesets ##

This method allows to specify multiple filesets with loose source files. Each fileset will be converted into a separate bulk build unit.


```xml

  <Library name="MultipleBBFilesets">
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
    <!--
    Split BB groups manually
    -->
    <bulkbuild>
      <sourcefiles name="BB_GROUP_1">
        <includes name="source/*4.cpp" />
        <includes name="source/*5.cpp" />
        <includes name="source/*6.cpp" />
      </sourcefiles>
      <sourcefiles name="BB_GROUP_2">
        <includes name="source/*1.cpp" />
        <includes name="source/*2.cpp" />
      </sourcefiles>
      <sourcefiles name="BB_GROUP_3">
        <includes name="source/*3.cpp" />
      </sourcefiles>
    </bulkbuild>
  </Library>

```
Same definition in traditional syntax:

 1. First you must supply a list of filesets to get bulkbuilt. To do this, specify a **runtime.bulkbuild.filesets** property.<br>Then set its value to be the filesets to be bulkbuilt.<br><br><br>```xml<br><!-- A package that doesn’t contain modules. --><br><property name="runtime.bulkbuild.filesets"><br>  anim  <!-- This will equate to: runtime.ea_aptaux_core.bulkbuild.anim.sourcefiles. --><br>  other<br></property><br><br><!-- A package that contains the ea_aptaux_core module. --><br><property name="runtime.ea_aptaux_core.bulkbuild.filesets"><br>  anim   <!-- This will equate to: runtime.ea_aptaux_core.bulkbuild.anim.sourcefiles. --><br>  other<br></property><br>```
 2. Then define the BulkBuild filesets. These filesets are actual source filesets.<br>Framework will automatically generate bulkbuild files for you. These files will be placed in the `${package.builddir}` <br><br><br>```xml<br><!-- A package that doesn’t contain modules. --><br><fileset name="runtime.bulkbuild.anim.sourcefiles"><br>  <includes name="${package.dir}/source/ea/aptaux/core/*.cpp"/><br></fileset><br><br><fileset name="runtime.bulkbuild.other.sourcefiles"><br><includes name="${package.dir}/source/ea/aptaux/core/*.cpp"/><br><excludes name="${package.dir}/source/ea/aptaux/core/anim*"/><br></fileset><br><br><!-- A package that contains the ea_aptaux_core module. --><br><fileset name="runtime.ea_aptaux_core.bulkbuild.anim.sourcefiles"><br>  <includes name="${package.dir}/source/ea/aptaux/core/*.cpp"/><br></fileset><br><br><fileset name="runtime.ea_aptaux_core.bulkbuild.other.sourcefiles"><br>  <includes name="${package.dir}/source/ea/aptaux/core/*.cpp"/><br>  <excludes name="${package.dir}/source/ea/aptaux/core/anim*"/><br></fileset><br>```


{{% panel theme = "danger" header = "Important:" %}}
The normal sourcefiles definition is completely ignored in this case unless [partial builkbuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/builkbuild_partial.md" >}} ) is specified.
{{% /panel %}}
