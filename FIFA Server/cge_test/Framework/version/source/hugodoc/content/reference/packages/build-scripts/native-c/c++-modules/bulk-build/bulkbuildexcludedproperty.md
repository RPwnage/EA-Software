---
title: bulkbuild.excluded property
weight: 113
---

Framework allows the exclusion of a subset of packages or modules from the bulkbuild.
Disabling bulkbuild for a subset of modules can improve iteration time when developer is working on modules containing large number of source files and needs to recompile
only few of those source files during each iteration.

<a name="Usage"></a>
## Usage ##

Define property  ** `bulkbuild.excluded` **  on nant command line, or in masterconfig  [globalproperties]( {{< ref "reference/master-configuration-file/globalproperties.md" >}} ) section. ** `bulkbuild.excluded` ** property can contain list of package names and/or module names in following format:


```
nant -D:bulkbuild.excluded="PackageA PackageB/Module1  PackageB/test/TestModule"
```
Formal definition of a  ** `bulkbuild.excluded` ** :


```
.          bulkbuild.excluded=
.                [package-name]
.                [package-name]/[module-name]
.                [package-name]/[group]/[module-name]
```
Where

 - `[package-name]` - name of the package. In this case all modules in the &#39;runtime&#39; group in this package are excluded from the bulkbuild.
 - `[package-name]/[module-name]` - In this case module &#39;[module-name]&#39;  in the &#39;runtime&#39; group of the package &#39;[dependent-package-name]&#39; is excluded from the bulkbuild.
 - `[package-name]/[group]/[module-name]` - In this case module &#39;[module-name]&#39;<br>in the &#39;[group]&#39; group of the package &#39;[package-name]&#39; is excluded from the bulkbuild.


##### Related Topics: #####
-  [bulkbuild.always property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildalwaysproperty.md" >}} ) 
