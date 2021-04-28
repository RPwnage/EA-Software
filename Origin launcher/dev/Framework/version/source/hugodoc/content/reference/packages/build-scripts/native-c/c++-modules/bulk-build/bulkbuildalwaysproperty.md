---
title: bulkbuild.always property
weight: 112
---

Framework allows developers to specify a subset of modules to be always built in bulk.
        
This allows developers or build farm maintainers to enable loose file builds in general but
force the bulk build of those that break in this configuration, perhaps if these are legacy
technologies that are due to eventually be sunset.

<a name="Usage"></a>
## Usage ##

Define property  ** `bulkbuild.always` **  on nant command line, or in masterconfig  [globalproperties]( {{< ref "reference/master-configuration-file/globalproperties.md" >}} ) section. ** `bulkbuild.always` ** property can contain list of package names and/or module names in following format:


```
nant -D:bulkbuild.always="PackageA PackageB/Module1  PackageB/test/TestModule"
```
Formal definition of a  ** `bulkbuild.always` ** :


```
.          bulkbuild.always=
.                [package-name]
.                [package-name]/[module-name]
.                [package-name]/[group]/[module-name]
```
Where

 - `[package-name]` - name of the package. In this case all modules in the &#39;runtime&#39; group in this package are excluded from the bulkbuild.
 - `[package-name]/[module-name]` - In this case module &#39;[module-name]&#39;  in the &#39;runtime&#39; group of the package &#39;[dependent-package-name]&#39; is excluded from the bulkbuild.
 - `[package-name]/[group]/[module-name]` - In this case module &#39;[module-name]&#39;<br>in the &#39;[group]&#39; group of the package &#39;[package-name]&#39; is always bulkbuilt.


##### Related Topics: #####
-  [bulkbuild.excluded property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludedproperty.md" >}} ) 
