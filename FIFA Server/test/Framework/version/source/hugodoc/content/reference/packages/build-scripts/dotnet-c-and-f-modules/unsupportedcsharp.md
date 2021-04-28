---
title: Building unsupported configs
weight: 179
---

This page describes how C# modules are handled when building with configs that do not support C#.
It also describes properties for controlling aspects of this behavior.

<a name="overview"></a>
## Overview ##

Building C# modules is not supported on every platform.
Framework only supports building C# modules on windows, and on Unix/OSX using mono.

In previous versions of framework, versions 3.28.00 and older, if a user tried to build a package with C# modules using a config that did not support C# the build would fail.
What they had to do was add a condition around the module so it would only be defined for those configs that supported C#.
However, this has been changed and now framework will automatically ignore C# modules if the config doesn&#39;t support them.

<a name="visualstudio"></a>
## Unsupported C# Modules in Visual Studio ##

The way that unsupported modules are handled in Visual Studio is a bit more complex.
When a solution is generated with only configs that do not support C# then by default the C# modules will not appear in the solution.
However, this can be controlled by setting the property &#39; `visualstudio.hide.unsupported.csharp` &#39; to  `false` .
Setting this property to false will make the C# modules visible for editing
but they will only be built when specifically trying to build the project individually
and will not be built when building all projects.

When a solution is generated for a mixture of configs that do and do not support C# the modules will always appear in visual studio.
However, when building all modules through visual studio they will only be built when a supported config is selected.

Building a solution via the command line should work the same as native NAnt builds and only build C# when it is supported.

