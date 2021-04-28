---
title: Utility modules
weight: 181
---

This topic describes the `Utility` module.

<a name="UtilityOverview"></a>
## Overview ##

Utility modules can be used to add nonbuildable modules to visual studio solutions.

When generating Visual Studio solution Framework will create a &#39;Utility&#39; Visual Studio project for each Utility module.

To define Utility module in a build script use the Utility structured XML block or set the `buildtype` to &#39;Utility&#39;.


```xml
<Utility name="MyModule">
```
Utility modules can also contain following build actions:

 - [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/utility-modules/prepostbuildsteps.md" >}} )
 - [Custom Build Steps]( {{< ref "reference/packages/build-scripts/utility-modules/custombuildsteps.md" >}} )
 - [Custom Build Files]( {{< ref "reference/packages/build-scripts/utility-modules/custombuildfiles.md" >}} )
 - [File Copy Steps]( {{< ref "reference/packages/build-scripts/utility-modules/filecopystep.md" >}} )

Files can be added to Utility modules. These files do not participate
directly in the build but can be used in generation of projects for external build systems.
For example, these files are added to Visual Studio projects as non-buildable files.

 - [excludedbuildfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/excludedbuildfiles.md" >}} )
 - [assetfiles]( {{< ref "reference/packages/build-scripts/utility-modules/assetfiles.md" >}} )

