---
title: What is Framework
weight: 2
---

This page explains what Framework is and why it is used by EA.

<a name="Introduction"></a>

Framework is a system for building code and data as well as a set of conventions which standardize how to perform these operations across EA.

At the heart of Framework is NAnt - an extensible commandline executable written in C# which executes a graph of XML based build scripts.
NAnt does not directly build code or data - it simply executes a series of Targets defined in Configuration Package and/or in your Build Script(s)
which specifies one or more processes known as Tasks to run (e.g., run the Visual Studio compiler on a group of .cpp files, run the file copy command
on a group of output files etc.). NAnt can be extended by creating custom Tasks which can then be executed in build scripts. Tasks also contain
attributes which are dynamically configured via user defined variables known as Properties. NAnt has several data structures (Property, FileSet, Optionset)
that can be defined in the Build scripts and a number of script language elements like &lt;if&gt;, &lt;do&gt;, &lt;foreach&gt;, etc that are used to control execution flow.

Another vital part of Framework is the Eaconfig configuration package. The configuration package is a set of XML Build scripts that are loaded by NAnt before executing a package Build script.
It is the Eaconfig package that imposes Framework standards, conventions and build logic.
Eaconfig defines Targets to build packages, generate Visual Studio Solutions, build visual Studio Solution, do distributed builds,
create documentation, and others. Eaconfig is what makes a general purpose build tool, such as NAnt, become a specialized system like Framework.

In addition to being a build system Framework is also a packaging system. Framework provides ways for developers to modularize their code into module which can be grouped
into packages that can be versioned and distributed. Framework build script can define modules as a set of source files that are compiled to single binary file.
A package is defined by Framework as a collection of build scripts that define modules that describes how they are build and how other packages and modules can depend on them.
Framework also defines the concept of a masterconfig file which describes which versions of packages to use and where Framework can find them.

## Why Framework? ##

Framework separates package definition, build options and package version management. Package definition describes source files, build type and other data.
If a package depends on other packages, the names of the dependent packages are listed in the build script, but not versions. Versions of the packages that will
be used in a build are specified in the Masterconfig.xml file. Masterconfig file also specifies the configuration package. The configuration package is a special
package that defines most of the build settings, such as compiler and linker flags, as well as the various targets executed during a build.

By creating a Framework compliant Package to build your content with NAnt, you gain the following nice features of Framework:

 - **Using the same set of package build scripts: perform build; generate Visual Studio Solution or XCode project; perform distributed build.**
 - **Versions for Dependant Packages can be updated in one file: masterconfig.xml so that Build Scripts don&#39;t need to be modified when the version(s) need to be changed.**
 - **A Package can be built for multiple target platforms without changes to the Package&#39;s Build Script.**
 - No PC environment tweaking is required to do a build. Environment variables, registry settings do not affect build results.<br>The results for builds performed on different PCs are identical regardless of local PC configurations. Missing system dependencies and environmental requirements can be detected by a build.

![Why Framework]( why_framework.png )