---
title: bulkbuild.exclude-writable
weight: 114
---

Framework can automatically exclude writable source files from the bulkbuild fileset.

<a name="Overview"></a>
## Overview ##

Writable files that are not located under the buildroot folder are automatically excluded from bulkbuild filesets and added as loose files.

This feature helps to keep the loose (non bulkbuild) builds in good shape while iterating on new code as it catches missing includes
that can cause issues when the bulkbuild files content gets reshuffled without paying the cost of the whole project being compiled without bulkbuild.
Once files uder development are submitted to the source control and become readonly they are automatically added back to bulkbuild fileset.

The feature can be turned on or off by property ** `bulkbuild.exclude-writable` ** . Default value for this property is  ** `true` ** .


##### Related Topics: #####
-  [bulkbuild.excluded property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludedproperty.md" >}} ) 
