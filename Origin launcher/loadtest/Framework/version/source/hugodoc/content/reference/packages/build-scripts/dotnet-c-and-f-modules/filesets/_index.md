---
title: FileSets
weight: 167
---

Filesets associated with DotNet modules

<a name="Section1"></a>
## DotNet module filesets ##

 - [assemblies]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/assemblies.md" >}} )
 - [assetfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/assetfiles.md" >}} )
 - [comassemblies]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/comassemblies.md" >}} )
 - [contentfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/contentfiles.md" >}} )
 - [excludedbuildfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/excludedbuildfiles.md" >}} )
 - [resourcefiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/resourcefiles.md" >}} )
 - [sourcefiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/sourcefiles.md" >}} )


{{% panel theme = "warning" header = "Warning:" %}}
Same file  should appear in one fileset. Framework will take settings from one fileset, and ignore duplicate file definitions.
{{% /panel %}}

##### Related Topics: #####
- {{< task "NAnt.Core.Tasks.FileSetTask" >}}
