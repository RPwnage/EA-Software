---
title: Make Style modules
weight: 187
---

This page describes the MakeStyle module, a module type with manually defined build and clean commands.

<a name="MakeStyleOverview"></a>
## Overview ##

MakeStyle modules are used to execute external build or clean commands.
You can define: `Build` ,  `Clean` , and  `ReBuild` commands.
These commands will be executed during nant build, or converted to corresponding type in generated projects. For example, when
generating Visual Studio solution Framework will create a &#39;Make Style&#39; Visual Studio project for each MakeStyle module.

To define a MakeStyle module use the MakeStyle Structured XML block for set the `buildtype` to &#39;MakeStyle&#39;


```xml
<MakeStyle name="MainModule">
```
MakeStyle module can contain following commands:

 - [MakeBuildCommand]( {{< ref "reference/packages/build-scripts/make-style-modules/makebuildcommand.md" >}} )
 - [MakeCleanCommand]( {{< ref "reference/packages/build-scripts/make-style-modules/makecleancommand.md" >}} )
 - [MakeRebuildCommand]( {{< ref "reference/packages/build-scripts/make-style-modules/makerebuildcommand.md" >}} ) - used only in Visual Studio


{{% panel theme = "info" header = "Note:" %}}
Working directory for all commands is set to `${package.builddir}.`
{{% /panel %}}
Several filesets can be specified for MakeStyle module as well. These filesets do not participate
directly in the build but can be used in generation of projects for external build systems.
For example, these files are added to Visual Studio projects as non-buildable files.

 - [sourcefiles]( {{< ref "reference/packages/build-scripts/make-style-modules/sourcefiles.md" >}} )
 - [asmsourcefiles]( {{< ref "reference/packages/build-scripts/make-style-modules/asmsourcefiles.md" >}} )
 - [headerfiles]( {{< ref "reference/packages/build-scripts/make-style-modules/headerfiles.md" >}} )
 - [assetfiles]( {{< ref "reference/packages/build-scripts/make-style-modules/assetfiles.md" >}} )


{{% panel theme = "info" header = "Note:" %}}
Legacy{{< url fileset >}} [excludedbuildfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/excludedbuildfiles.md" >}} ) is also supported
{{% /panel %}}
