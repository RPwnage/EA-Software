---
title: Partial Bulk Build
weight: 111
---

Partial bulkbuilds include both bulkbuild files and loose source files that are not included into bulk build file(s)

<a name="PartialBulkbuilds"></a>
## Partial bulkbuilds ##

By default regular source file filesets are completely ignored and excluded from build.
This is done for performance reasons. build target assumes that bulk build filesets contain all source files.

###### Use partial attribute in SXML ######

```xml

  <Library name="BulkBuild_ExcludeSourceFiles">
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
    <!--
       Some source files can be excluded from bulkbuild.
       Add 'loosefiles' fileset and enable 'partial' bulkbuild
    -->
    <bulkbuild enable="true"  partial="true">
      <loosefiles>
        <includes name="source/*4.cpp" />
      </loosefiles>
    </bulkbuild>
  </Library>

```
In traditional framework syntax ff you want to have some loose files in addition to bulk build files use following properties:

###### To enable partial bulkbulds for all modules in the package ######

```xml
<property name="package.enablepartialbulkbuild" value="true"/>
```
###### To enable partial bulkbulds for a single module in the package ######

```xml
<!-- Package with modules: -->
<property name="[group].[module].enablepartialbulkbuild" value="true"/>

<!-- Package without modules: -->
<property name="[group].enablepartialbulkbuild" value="true"/>
```
It will work for **.bulkbuild.filesets** ,  **.bulkbuild.sourcefiles**  and  **.bulkbuild.manual.sourcefiles** .
Loose files will be computed as difference between all source files and all bulkbuild files:

loose = all source files - all bulk build files
```xml
<property name="package.enablepartialbulkbuild" value="true"/>
          
<fileset name="runtime.sourcefiles">
  <!--List out the files that won't bulk build individually-->
  <includes name="Nonbulkbuildfile.cpp"/>
  …etc
</fileset>
          
<fileset name="runtime.bulkbuild.sourcefiles">
  <!--Place your bulkbuild files here-->
</fileset>


<property name="package.enablepartialbulkbuild" value="true"/>
          
<fileset name="runtime.sourcefiles">
  <!--List out the files that won't bulk build individually-->
  <includes name="Nonbulkbuildfile.cpp"/>
  …etc
</fileset>
          
<fileset name="runtime.bulkbuild.A.sourcefiles">
  <!--Place your bulkbuild files here-->
</fileset>
          
<fileset name="runtime.bulkbuild.B.sourcefiles">
  <!--Place your bulkbuild files here-->
</fileset>
          
<property name="runtime.bulkbuild.filesets" value="A B" />
```
