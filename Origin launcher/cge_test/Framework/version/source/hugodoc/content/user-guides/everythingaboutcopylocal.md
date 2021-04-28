---
title: Copy Local In Framework
weight: 36
---

This topic explains the copy local features within Framework.

<a name="WhatIsCopyLocal"></a>
## What is copy local? ##

Copy Local is the term given to any feature in Framework involved in copying files from one package or module to the output folder of another. This
includes copying prebuilt file inside a distributed package as well as copying the built output of a dependency to the output folder of its parent.
The most common use for copy local is to have Framework automatically copy shared library dependencies to the output folder of a top level executable.

<a name="CopyLocalDefinitions"></a>
## Defining A Copy Local Relationship ##

<a name="CopyLocalModule"></a>
### Copy Local Module ###

The most common and useful method for defining copy local is to declare a module as copy local. This is done by in structured xml using the [copy local element]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/copylocal.md" >}} ) .

###### Copy Local Module Example ######

```xml
<Module name="MyModule">
 <copylocal>true</copylocal>
  or
 <copylocal value="true"/>
</Module>
```
A copy local module will transitively scan its dependency graph for [copy local files]( {{< ref "#CopyLocalFiles" >}} ) and then setup the appropriate post build actions
to copy those files to the module&#39;s output directory.

<a name="CopyLocalDependency"></a>
### Copy Local Dependency ###

A module that has a build or link dependency on another module can declare the dependency as a copy local dependency. Copy local dependencies are
a legacy feature and it is advised that you use Copy Local Modules instead.

###### Copy Local Dependency Example ######

```xml
<Module name="MyModule">
 <dependencies>
  <auto copylocal="true">
   MyOtherPackage/MyOtherModule
  </auto>
 </dependencies>
</Module>
```
A copy local dependency defines a single level dependency between a module and its dependency. The [copy local files]( {{< ref "#CopyLocalFiles" >}} ) from the child in the dependency will be copied to the parent. Dependencies of this type are also a transitive - if the child has a dependency on a third
module then files from the third module will copied to the parent as well. If a parent module has a copy local dependency on a [
					copy local module]( {{< ref "#CopyLocalModule>" >}} ) then all files from the dependency subgraph of the dependency will be copied to the parent.

<a name="MarkingFilesNonCopyLocal"></a>
### Marking Files Non-Copy Local ###

In some cases it may be useful to mark particular files to be ignored by copy local. Most commonly this can be achieved via the &quot;asis&quot; option on filesets which,
in addition to its other effects, makes the copy local transitive scan ignore these files.

###### Asis Fileset Example ######

```xml
<publicdata packagename="MyPackage">
 <module name="MyModule">
  <dlls-external>
   <includes name="MyDll.dll" asis="true"/>
  </dlls-external>
 </module>
</publicdata>
```
You can also use the copylocal option. This options only applies to filesets found by copy local scan, namely dlls, assemblies and content files.

###### Copy Local Off Optionset Example ######

```xml
<optionset name="no-copy-local">
 <option name="copylocal" value="off"/>
</optionset>

<Program name="MyProgram">
 <copylocal>true</copylocal>
 <dlls basedir="external">
  <includes name="ExternalDll.dll" optionset="no-copy-local"/>
 </dlls>
</Program>
```
<a name="CopyLocalFiles"></a>
## Copy Local Files ##

The following files are considered candidates for copy local:

 - .NET Assemblies (.dll, .exe)
 - Content Files
 - Native Shared Libraries (.dll, .prx, .so, etc)
 - MS Debug Symbols from .NET Assemblies or Native Shared Libraries (.pdb)
 - Native Executables

<a name="Use Hard Links To Reduce Footprint"></a>
## Using Hard Links To Reduce Footprint ##

In large projects with extensive dependency graphs having every module build to a separate directory and then copy it&#39;s output to the final
output directory can be time consuming and consume a large amount of disk space. Framework has the option to enable hard linking where possible
for copy local operations. This can be enabled via the copy local element in structured xml:

###### Hard Linking Example ######

```xml
<Module name="MyModule">
 <copylocal value="true" use-hardlink-if-possible="true"/>
</Module>
```
<a name="PostBuildCopyLocal"></a>
## Post Build Copy Local ##

Post build copy local is an workflow optimization that makes a module responsible for performing copies of its outputs to modules that depend on it.
It is enabled via the nant.postbuildcopylocal global property in the masterconfig.

###### Masterconfig Post Build Copy Local Example ######

```xml
<globalproperties>
 nant.postbuildcopylocal=true
</globalproperties>
```
Normally a copy local operation is owned by the target of the copy i.e if module A depends on module B it is the responsilbity of module A to copy
module B&#39;s outputs to module A&#39;s output directory. By enabling post build copy local it also becomes the responsiblity of module B to perform this
copy.

This feature is intended to be used in conjunction with build system that allow targeted rebuilds to prevent unnecessary rebuilding. Consider a
scenario where a .NET program A depends on a .NET library B. If an internal change is made to library B that does not affect the interface to A
then it shouldn&#39;t be necessary to rebuild A. However most dependency checking systems are not capable of detecting this and so will rebuild A
in case B has changed it&#39;s interface. This can be avoided by performing a targeted rebuild of just B. However, A is responsible for copying B&#39;s
output to its output directory, because A has not been built it will still load a old copy of B that doesn&#39;t contain the change. By reversing
the copy responsbility it can be made so that rebuilding just B does update the compiled version of B in A&#39;s output directory.

<a name="CopyLocalMechanisms"></a>
## Copy Local Mechanisms ##

<a name="NantBuild"></a>
### NAnt Build ###

Copies in NAnt build are performed using the &lt;copy&gt; task as if it were used in NAnt script.

<a name="MakeBuild"></a>
### Make Build ###

Copies in generated make files are performed using the copy.exe that ships with Framework package.

<a name="Visual Studio / MSBuild"></a>
### Visual Studio / MSBuild ###

Copies in Visual Studio or via MSBuild are performed using a custom MSBuild copy task. This is used in favour of Visual Studio&#39;s copy
local mechanism.

Be aware that when examining references in Visual Studio the Copy Local field on the property page will be set to false. This can be
ignored as copy local is being handled in a custom way. To see which files are being copied by copy local look at the FrameworkCopyLocal
item group in the generated project. In .NET projects the Solution Explorer will contain a CopyLocalFiles folder that show all the files
that will be copied by the custom mechanism.


##### Related Topics: #####
-  [copylocal]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/copylocal.md" >}} ) 
