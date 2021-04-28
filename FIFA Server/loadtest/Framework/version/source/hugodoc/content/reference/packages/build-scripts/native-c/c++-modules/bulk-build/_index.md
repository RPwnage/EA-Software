---
title: Bulk Build
weight: 108
---

Bulkbuild is build optimization by combining multiple source files into a single compilation unit file using `#include` statements.

The topics in this section describe how ho define bulk build input data and control generation of the bulkbuild files.

<a name="BulkBuildOverview"></a>
## Usage ##

Bulkbuild can be turned on or off using [bulkbuild property]( {{< ref "reference/properties/bulkbuild_property.md" >}} ) , by default  **bulkbuild** is &#39;on&#39;.

 **bulkbuild** property turns bulkbuild &#39;on&#39; or &#39;off&#39; in all packages. When bulkbuild is &#39;on&#39; globally it is possible to exclude
subset of packages or modules from the bulkbuild by using [bulkbuild.excluded property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludedproperty.md" >}} ) 

 - [Auto Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuild_auto.md" >}} )
 - [Pre-generated Bulk Build ]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuild_pregenerated.md" >}} )
 - [Partial Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/builkbuild_partial.md" >}} )
 - [bulkbuild.exclude-writable]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludewritable.md" >}} )




{{% panel theme = "info" header = "Note:" %}}
1. For a given package, you can only use one of these methods listed above to bulkbuild.<br>Specifying both **runtime.bulkbuild.filesets**  and  **runtime.bulkbuild.sourcefiles** (or **runtime.bulkbuild.manual.sourcefiles** ) is an error.
 2. The **bulkbuild** property is special. It now gets passed down through all the packages builddependencies.<br>So all builddependencies will be built using bulkbuild.
 3. If the bulkbuild property is set and you do not specify either the **.bulkbuild.filesets**  or  **.bulkbuild.sourcefiles** property then the package will be built without using bulkbuild. i.e Build all the files in the **.sourcefiles** fileset
 4. If a package is buildable using BulkBuild, then it must be buildable using the both the bulkbuild and non-bulkbuild methods.
 5. The BulkBuild system is buildable using both NAnt and Visual Studio. The bulkbuild files are added to the vcproj.<br>The source files used in the bulkbuild files will also be added to the vcproj but will be marked as not buildable.
{{% /panel %}}

##### Related Topics: #####
-  [bulkbuild property]( {{< ref "reference/properties/bulkbuild_property.md" >}} ) 
