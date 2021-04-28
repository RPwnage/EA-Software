---
title: A Brief History of Framework
weight: 3
---

Framework has been around for a long time. The first version of Framework was published to the package server in 2002.
Over the years Framework has undergone a few drastic changes.
This page provides a brief overview of some of the history of Framework so that those new to Framework can understand
how it has evolved and why some of the legacy features that are mentioned in this guide still exist.

<a name="Framework123"></a>
## Framework 1, 2, 3 - what is the difference? ##

Throughout this guide you may encounter termsFramework 1, Framework 2, or Framework 3+.
     Framework has a long history at EA and these three version numbers were major reworks of Framework based on usage experience and new demands.
Framework has been much more stable since Framework 3 and increases in the major version since then have only indicated when there are significant breaking changes but there has not been a significant rework since Framework 3.

All Framework implementations are based on NAnt, but Framework works and how build scripts are written has changed through the versions.
Nonetheless each subsequent implementation of the Framework is backwards compatible (with some minor restrictions) and supports packages written for a previous version of Framework.

 - Framework 1<br><br>In Framework 1 each{{< url FrameworkPackage >}}contained explicit commands how to build it.<br>The build script contained implementation of the build target that would build the{{< url FrameworkPackage >}}and all its dependent packages.<br>Versions of dependent packages were specified in the parent package build script.  When Framework 1 was written, it was common practice to distribute<br>pre-built libraries and binaries as part of the package distribution; publishing source was considered optional.<br><br>Framework 1 approach to code builds contained number of maintenance challenges and restrictions:<br><br>  - Updating versions of{{< url FrameworkPackage >}}s required editing of other {{< url FrameworkPackage >}}s build scripts.<br>  - Because all that was required from a{{< url FrameworkPackage >}}is to be able to build itself, it was difficult to convert a Framework<br>build script into input to some other build system (for example, it could not be used to generate a visual studio solution).<br><br>Starting in Framework 7 backward compatibility with Framework 1 style packages has been dropped and Framework 1 only features have been removed.<br>This was done to reduce the number of legacy features that Framework needs to support as the CM team plans the future of Framework.

 - Framework 2<br><br>As our CM processes evolved, it became common practice to publish source code and instructions for building it rather than pre-built libraries.<br>This presented a number of challenges to the Framework 1 model, so Framework 2 was created to address them.  Packages written for Framework 2 use<br>adescriptive instead of imperativeapproach to their build scripts.<br><br>  - In Framework 2,{{< url FrameworkPackage >}}build scripts describe the package content and provide build instructions<br>rather than doing the build itself.<br><br>The task of building was delegated to the common targets implemented in the eaconfig Package. The information collected from each package<br>build script could then be used to do a build or to convert this information into a Visual Studio Solution, or, potentially, another format.<br>  - Another big step in Framework 2 is introduction of the{{< url Masterconfig >}}. Masterconfig file now contained versions of all Packages involved, and it become<br>much easier to perform updates.<br><br>Framework 2 was big step forward compared to Framework 1, but it had number of problems that were addressed in Framework 3:<br><br>  - Recursive nature of Framework 2 inherited from Framework 1 made it difficult to collect build information and convert it to other build system formats.nantToVSToolsPackage used to convert Framework build scripts into Visual Studio solutions became extremely complicated, fragile and hard to maintain.<br>  - Framework 2 was not flexible enough to easily accommodate new complex build pipeline of platforms such as Android, iOS, and others,<br>and a lot of hacks were introduced to make Framework work on those platforms. These hacks made the whole system very fragile and hard to maintain.<br>  - Framework 2 was not fast enough. There were several optimizations of the Framework 2 over past few years, but improvements in speed were not catching up with<br>increased size and complexity of our builds<br>  - Framework 2 style build scripts were verbose, confusing and hard to debug. Modules in Framework 2 were simply a bunch of properties, filesets and optionsets<br>that followed a specific naming convention. In some cases these xml elements would be split across multiple build files or defined in loops.<br>They were also more difficult to write and debug because users would need to remember the name of the properties and if they made a typo it would be hard for Framework to catch.<br><br>The CM team stopped supporting Framework 2 in 2013 and all users are now expected to be running Framework 3 or later.<br>However, Framework 2 style packages and build scripts are still entirely backward compatible with the most recent versions of Framework.

 - Framework 3<br><br>Framework 3 uses the same build script interfaces and{{< url Masterconfig >}}files as Framework 2, and it addresses many of the problems<br>that were found in Framework 2 by implementing completely different internal flow for the build process:<br><br>  - Framework 3 abandons recursive approach to builds used in Framework 2.<br>Package script files are loaded and converted into C# objects organized in a graph.<br>Tasks in Framework 3+ can use theBuild Graphto perform NAnt build or write build graph in different formats.<br>  - Framework 3 version of NAnt was significantly reworked to be thread safe, and concurrency was introduced at all stages of the build process.<br>As result both native NAnt builds and solution generation are significantly faster than in Framework 2.<br>  - Framework 3 supports transitive dependency propagation, which should reduce package maintenance effort and should help to reduce number of Packages<br>that aren&#39;t needed anymore but still present in game build scripts because of explicit dependencies declared by top level packages.<br>  - Structured XML was introduced in Framework 3. In Framework 2 modules were defined using a variety of properties, fileset and optionsets.<br>In Structured XML modules are a block of XML with nested elements. Since modules are now defined in a single block it is more difficult to define them across multiple files or define a bunch of modules in a loop.<br>It also made writing and debugging build scripts easier because users could use intellisense to auto-complete fields in their build script and Framework would throw better warnings when elements were defined incorrectly.

<br>###### This is two modules defined in the Framework 2 style ######<br><br>

```xml
<property name="runtime.buildmodules" value="LibModule ProgramModule"/>

<property name="runtime.LibModule.buildtype" value="Library" />

<fileset name="runtime.LibModule.sourcefiles">
    <includes name="${package.dir}/source/lib/*.cpp"/>
</fileset>

<property name="runtime.ProgramModule.buildtype" value="Program" />

<property name="runtime.ProgramModule.runtime.moduledependencies">
    LibModule
</property>

<fileset name="runtime.ProgramModule.sourcefiles">
    <includes name="${package.dir}/source/prog/*.cpp"/>
</fileset>

```
<br>###### This is the same two modules defined in Structured XML ######<br>

```xml
<Library name="LibModule">
    <sourcefiles>
        <includes name="${package.dir}/source/lib/*.cpp"/>
    </sourcefiles>
</Library>

<Program name="ProgramModule">
    <dependencies>
        <auto>
            MyPackage/LibModule
        </auto>
    </dependencies>
    <sourcefiles>
        <includes name="${package.dir}/source/lib/*.cpp"/>
    </sourcefiles>
</Program>
```
 - Framework 4+Following the release of Framework 3, Framework became much more stable and there was not the same need for a major rework.<br>However, the versioning scheme used by Framework package typically consists of three fields: major version, minor version and fix version.<br>The CM team eventually reached a point where they wanted to indicate to their users using the major version that there were breaking changes.

