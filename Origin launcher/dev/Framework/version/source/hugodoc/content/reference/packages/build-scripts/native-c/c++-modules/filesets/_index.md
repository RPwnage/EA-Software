---
title: Filesets
weight: 135
---

Filesets associated with Native CPP modules

<a name="Section1"></a>
## Native CPP module filesets ##

 - [asmsourcefiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/asmsourcefiles.md" >}} )
 - [sourcefiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/sourcefiles.md" >}} )
 - [headerfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/headerfiles.md" >}} )
 - [libs]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/libs.md" >}} )
 - [objectfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/objectfiles.md" >}} )
 - [dlls]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/dlls.md" >}} )
 - [resourcefiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/resourcefiles.md" >}} )
 - [assemblies]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/assemblies.md" >}} )
 - [excludedbuildfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/excludedbuildfiles.md" >}} )
 - [bulkbuild.sourcefiles and bulkbuild.filesets]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/builkbuildfiles.md" >}} )
 - [shaderfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/shaderfiles.md" >}} )


{{% panel theme = "warning" header = "Warning:" %}}
Same file should appear in one fileset. Framework will take settings from one fileset, and ignore duplicate file definitions.
{{% /panel %}}

##### Related Topics: #####
- {{< task "NAnt.Core.Tasks.FileSetTask" >}}
