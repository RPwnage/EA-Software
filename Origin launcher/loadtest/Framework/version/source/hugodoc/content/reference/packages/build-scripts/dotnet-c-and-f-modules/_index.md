---
title: DotNet (C# and F#) modules
weight: 147
---

DotNet (C#, F#) modules

<a name="DotNetModulesOverview"></a>
## Overview ##

DotNet modules are supported in both Visual Studio and nant builds, but some features are only available when building using Visual Studio.

To define DotNet module in a build script use the appropriate structured XML block or set the set `buildtype` to one of the standard DotNet buildtypes provided by eaconfig or custom buildtype derived from them.


```xml
<CSharpLibrary name="MyModule" >
```
DotNet module specific Topics:

 - [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/prepostbuildsteps.md" >}} )
 - [Files with CustomTool in Visual Studio]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/fileswithcustomtools.md" >}} )
 - [Add attributes and nested elements to Xaml files]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/addattributesandadditionalnestedelementstoxaml.md" >}} )
 - [T4 text templates (.tt files)]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/t4texttemplates.md" >}} )
 - [Building unsupported configs]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/unsupportedcsharp.md" >}} )
 - [Generated Assembly Info Files]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/generatedassemblyinfo.md" >}} )

DotNet module properties

 - [copylocal]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/copylocal.md" >}} )
 - [targetframeworkversion]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/targetframeworkversion.md" >}} )
 - [rootnamespace]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/rootnamespace.md" >}} )
 - [application-manifest]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/application-manifest.md" >}} )
 - [appdesigner-folder]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/appdesigner-folder.md" >}} )
 - [disablevshosting]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/disablevshosting.md" >}} )
 - [generateserializationassemblies]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/generateserializationassemblies.md" >}} )
 - [importmsbuildprojects]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/importmsbuildprojects.md" >}} )
 - [runpostbuildevent]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/runpostbuildevent.md" >}} )
 - [Generate Docs]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/generatedocs.md" >}} )
 - [win32icon]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/win32icon.md" >}} )

Setting Workflow and Unittest modules

 - [workflow]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/workflow.md" >}} )
 - [unittest]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/unittest.md" >}} )
 - [webapp]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/webapp.md" >}} )

Compiler options

 - [defines]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Defines" >}} )
 - [warningsaserrors]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Warningsaserrors" >}} )
 - [suppresswarnings]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Suppresswarnings" >}} )
 - [debug]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Debug" >}} )
 - [keyfile]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Keyfile" >}} )
 - [platform]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Platform" >}} )
 - [allowunsafe]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#Allowunsafe" >}} )
 - [csc-args/fsc-args]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md#AdditionalArgs" >}} )

DotNet module filesets

 - [excludedbuildfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/excludedbuildfiles.md" >}} )
 - [sourcefiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/sourcefiles.md" >}} )
 - [assemblies]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/assemblies.md" >}} )
 - [resourcefiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/resourcefiles.md" >}} ) <br><br> [resourcefiles.notembed]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/resourcefiles.md" >}} ) <br><br>property [resourcefiles.basedir]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/resourcefiles.md" >}} ) <br><br>property [resourcefiles.prefix]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/resourcefiles.md" >}} )
 - [contentfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/contentfiles.md" >}} )

Web references

 - [webreferences]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/webreferences.md" >}} )

COM assemblies

 - [comassemblies]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/comassemblies.md" >}} )

