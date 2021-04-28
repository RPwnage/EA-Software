---
title: Build Scripts
weight: 84
---

This section describes the format of build scripts and the various types of modules used to describe a package.

<a name="Overview"></a>
## Overview ##

Package `*.build` files are XML documents that must follow a specific structure.
Build files have several elements that are required and others that are optional.

A common group of elements found in build files are in the form of lists. These elements contain a list
of file names, options, variables, etc. The three main types of list elements used in build scripts are,{{< task "NAnt.Core.Tasks.PropertyTask" >}},{{< task "NAnt.Core.Tasks.FileSetTask" >}}, and{{< task "NAnt.Core.Tasks.OptionSetTask" >}}.

Build files may also contain elements which represent a [Task]( {{< ref "reference/tasks/_index.md" >}} ) or [Target]( {{< ref "reference/targets/_index.md" >}} ) and whose nested elements are executed sequentially.
One common example of this is pre- and post-build targets.


{{% panel theme = "info" header = "Note:" %}}
NAnt does not have any means to enforce conventions on property, fileset, or optionset names,
however Framework has some naming conventions that must be followed to get correct build results.
Script code that appears outside of Target or Task definitions is executed when the build file is loaded.
{{% /panel %}}
<a name="RequiredElements"></a>
### Required Elements ###

Package build script must contain the `project` element as their root element and
within the project element they must contain a{{< task "EA.FrameworkTasks.PackageTask" >}}task.


{{% panel theme = "info" header = "Note:" %}}
While the package task is required the name and version number are taken from the package&#39;s directory name
and the information provided in the build script is only used if the package lives outside of this directory structure.
{{% /panel %}}

```xml
<project default="build">
 <!-- package task loads configuration package (eaconfig) -->
 <package name="MyPackage" targetversion="1.00.00"/>
 . . . . . . . .
</project>
```
Buildable packages may be divided into{{< url Module >}}s, which represent a build unit: library, program, etc.

When a package is divided into modules the build script must contain a property named{{< url buildgroup >}} `.buildmodules` containing a list of all the modules.

Each module must provide a property named{{< url groupname >}} [.buildtype]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) .
The Buildtype value is the name of an optionset which is used to determine the module type: &#39;Library&#39;, &#39;Program&#39;, &#39;Utility&#39;, &#39;MakeStyle&#39;, etc.
This Buildtype optionset also contains number of flags, compiler and linker options, and other data that are used during the build.


{{% panel theme = "info" header = "Note:" %}}
Package build scripts should contain module descriptions, it is not recommended to execute targets or script
code that is part of the build process. This should be accomplished in [pre/post process]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md" >}} ) steps.
{{% /panel %}}
<a name="Example"></a>
### Example Build Script ###

Here is a fragment of a build script showing how to declare packages and modules.


```xml
<project default="build" xmlns="schemas/ea/framework3.xsd">
 <package name="MyPackage" />

 <Library name="MyLibraryModule">
 . . . . . . . .
 Add properties and filesets to describe this Library module
 . . . . . . . .
 </Library>

 <Utility name="MyUtilityModule">
 . . . . . . . .
 Add properties and filesets to describe this Utility module
 . . . . . . . .
 </Utility>
</project>
```
<a name="OtherBuildScriptElements"></a>
### Other Build Script Elements ###

Other some other categories of build script elements

 - [Dependencies]( {{< ref "reference/packages/build-scripts/dependencies/_index.md" >}} )
 - [Pre/Post Process Steps]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md" >}} )

Information about specific types of modules, including their specific properties, fileset and optionsets.

 - [Native (C/C++) modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/_index.md" >}} )
 - [DotNet (C#) modules]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/_index.md" >}} )
 - [Utility modules]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} )
 - [Make Style modules]( {{< ref "reference/packages/build-scripts/make-style-modules/_index.md" >}} )
 - [External Visual Studio Project modules]( {{< ref "reference/packages/build-scripts/externalvisualstudio.md" >}} )

