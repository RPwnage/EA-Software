---
title: Solution Generation Questions
weight: 309
---

This page lists some commonly asked questions related to solution generation.

<a name="RelativePaths"></a>
## Is it possible to generate a solution with all relative paths? or generate a portable solution? ##

By default framework will try to use relative paths for source files included in a visual studio solution.
In some cases where it is unable to do so it will fall back to using absolute paths.

To make a solution more portable set the property [eaconfig.generate-portable-solution]( {{< ref "reference/visualstudio/generate_portable_solution.md" >}} ) to true.
This will use environment variables for SDK paths and try to use relative paths to tools so that the solution is not tied to a particular machine.

<a name="CustomProjectFiles"></a>
## Can I modify solution generation to inject a specific line of xml into a project file? ##

Framework provides a number of different ways to modify how elements are written to a project file,
including a way to write a C# script that can write a specific line to the file.
We have a page that describes each of the ways in detail here: [Visual Studio Custom Project File Elements]( {{< ref "reference/visualstudio/vsconfigbuildoptions.md" >}} ) 

