---
title: Pre-generated Bulk Build 
weight: 110
---

Pregenerated bulk build files means that bulk build file is already created. For native NAnt builds
it would be enough to add pre-generated bulk build file to the sourcefiles fileset, but to have original
source files added to Visual Studio solution files when using solution generation targets use the following method.

<a name="Section1"></a>
## Pre-generated bulkbuild file ##

For native NAnt builds it would be enough to add pre-generated bulk build file to the sourcefiles fileset, but to have original,
loose, source files added to Visual Studio solution files when using solution generation targets use the following method.


```xml

  <Library name="PregeneratedBBFiles">
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
    <!--
    Use manually created bulk build files instead of generating them
    -->
    <bulkbuild>
      <manualsourcefiles>
        <includes name="bb_source\bb_sources.cpp" />
      </manualsourcefiles>
    </bulkbuild>
  </Library>

```
Same definition in traditional syntax:

 - Add loose source files to the **.sourcefiles** fileset.<br><br><br>{{% panel theme = "danger" header = "Important:" %}}<br>The normal sourcefiles definition is completely ignored in the build unless [partial builkbuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/builkbuild_partial.md" >}} ) is specified.<br>{{% /panel %}}
 - Add pre-generated bulkbuild file(s) to the **.bulkbuild.manual.sourcefiles** fileset.


```xml
<!-- A package that doesn’t contain modules. -->
<fileset name="runtime.bulkbuild.manual.sourcefiles">
  <includes name="bbsource/*.cpp"/>
</fileset>

<!-- A package that contains the ea_aptaux_core module. -->
<fileset name="runtime.ea_aptaux_core.bulkbuild.manual.sourcefiles">
  <includes name="bbsource/*.cpp"/>
</fileset>
```
