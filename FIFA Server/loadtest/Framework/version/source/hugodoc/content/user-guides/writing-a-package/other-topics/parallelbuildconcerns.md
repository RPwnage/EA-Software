---
title: Parallel Build Concerns
weight: 21
---

Your build is usually being built in parallel (either in Visual Studio, Incredibuild, etc).  So
it is important that your build script do not have custom actions that could break parallel builds.
This section attempts to list things that people sometimes get tripped up with and what might
be a possible alternative of doing the same things.

<a name="CommonPreBuildStep"></a>
## Multiple Modules Creating Same File On Pre-Build Step ##

Sometimes you may end up in a situation where for a specific module you want to execute
a custom tool to generate files in a pre build step.  It is important to make sure
that you don&#39;t have multiple modules having pre build step trying to do the same thing,
otherwise, during parallel build, you could have two modules trying to create the same set of
files and you could end up having build error.  In situations like this, it is better to move
these common tasks out of pre build step into an utility module (such as a &quot;MakeStyle&quot; module)
and have your two modules depend on it.  The following is an example (you can also find this in the
example folder).  Please see [Utility modules]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) and [Make Style modules]( {{< ref "reference/packages/build-scripts/make-style-modules/_index.md" >}} ) pages for more info.


```xml
<package name="MakestyleModuleEx"/>

<MakeStyle name="CreateHeader">
  <MakeBuildCommand>
    @echo Executing runtime.CreateHeader.MakeBuildCommand
    if not exist &quot;${package.MakestyleModuleEx.builddir}\${config}\Generated&quot; mkdir &quot;${package.MakestyleModuleEx.builddir}\${config}\Generated&quot;
    echo inline const char *GetConfigTestString() { return &quot;${config}&quot;; } > &quot;${package.MakestyleModuleEx.builddir}\${config}\Generated\BuildConfig.h&quot;
    @echo Finished runtime.CreateHeader.MakeBuildCommand
  </MakeBuildCommand>

  <MakeCleanCommand>
    @echo Executing runtime.CreateHeader.MakeCleanCommand
    @if exist &quot;${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h&quot; del /F /Q &quot;${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h&quot;
    @echo Finished runtime.CreateHeader.MakeCleanCommand
  </MakeCleanCommand>

  <MakeRebuildCommand>
    @echo Executing runtime.CreateHeader.MakeRebuildCommand
    @if exist &quot;${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h&quot; del /F /Q &quot;${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h&quot;
    if not exist &quot;${package.MakestyleModuleEx.builddir}\${config}\Generated&quot; mkdir &quot;${package.MakestyleModuleEx.builddir}\${config}\Generated&quot;
    @echo inline const char *GetConfigTestString() { return &quot;${config}&quot; } > &quot;${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h&quot;
    @echo Finished runtime.CreateHeader.MakeRebuildCommand
  </MakeRebuildCommand>
</MakeStyle>

<Program name="HelloWorld">
  <dependencies>
    <auto>
      MakestyleModuleEx/CreateHeader
    </auto>
  </dependencies>
  <sourcefiles basedir="${package.MakestyleModuleEx.dir}/source">
    <includes name="hello_world.cpp"/>
  </sourcefiles>
  <includedirs>
    ${package.MakestyleModuleEx.builddir}/${config}/Generated
  </includedirs>
  <headerfiles>
    <includes asis="true" name="${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h"/>
  </headerfiles>
</Program>

<Program name="HelloWorld2">
  <dependencies>
    <auto>
      MakestyleModuleEx/CreateHeader
    </auto>
  </dependencies>
  <sourcefiles basedir="${package.MakestyleModuleEx.dir}/source">
    <includes name="hello_world2.cpp"/>
  </sourcefiles>
  <includedirs>
    ${package.MakestyleModuleEx.builddir}/${config}/Generated
  </includedirs>
  <headerfiles>
    <includes asis="true" name="${package.MakestyleModuleEx.builddir}/${config}/Generated/BuildConfig.h"/>
  </headerfiles>
</Program>
```
<a name="CommonPostBuildFileCopy"></a>
## Multiple Modules Doing Same File Copy On Post-Build Step ##

Similar to the pre-build step, you have similar concern with post-build step.  Sometimes, your module might have
post build step to copy your dependent package&#39;s DLL, or your game assets into your bin folder.  However, if you have
multiple modules trying to do the same thing, you will run into situation where two modules trying to copy the same file
to the same location.

For copying dependent package&#39;s DLL, you should update your build script to use the &quot;CopyLocal&quot; support
and let Framework perform this action for you.  Please see [Copy Local In Framework]( {{< ref "user-guides/everythingaboutcopylocal.md" >}} ) page
for more information.

For copying asset files, you can also use the &quot;assetfiles&quot; fileset specification to instruct Framework to do
the file copying for you.  Please see [assetfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/assetfiles.md" >}} ) page for more information.

<a name="ParallelMultiConfigBuild"></a>
## Building Multiple Configurations At The Same Time (buildall target or solution generation targets) ##

Aside from making sure that your pre-build and post-build steps don&#39;t cause conflict between
the different modules, you also should keep in mind that those pre-build and post-build steps of each
module won&#39;t create conflict between different configurations during build or solution generation.

When you execute the &quot;buildall&quot; target (or &quot;slnruntime&quot; targets), the different configuration builds are
going to be executed in parallel in Framework.  So it is important to keep in mind that even for a single module,
you do not dynamically create files in the path that will cause conflict with different configuration build.
In the above [Makestyle Module example]( {{< ref "user-guides/writing-a-package/other-topics/parallelbuildconcerns.md#CommonPreBuildStep" >}} ) ,
you notice that we output the &quot;BuildConfig.h&quot; file to a ${config} specific folder.

Aside from parallel build behaviour of the &quot;buildall&quot; target (or &quot;slnruntime&quot; targets), keeping those
output files to ${config} specific folder also help remove unexpected behaviour doing incremental build when you
switch your build from one config to another config.  However, if your output files are supposed to be identical to all
configs and want to use the same outupt location, then you need to make sure that your code gen application can handle
being executed in parallel and potentially doing same output to the same location.

<a name="CommonFileCopy"></a>
## Multiple Modules Trying To Do The Same File Copy ##

There are times where you have multiple modules definition needing to do file copy of exact same set of data
to the exact same output location.  In Framework version AFTER 3.28.00 release, you can use the `filecopystep` element of an `Utility` module to do this special file copies and have your other modules
depend on it.


```xml
<package name="CommonFileCopyEx"/>

<Utility name="CopyCommonDataFiles">
    <filecopystep todir="${package.configbindir}">
        <fileset>
            <includes fromfileset="package.SomePackage.filesetname"/>
        </fileset>
    </filecopystep>
</Utility>

<Program name="HelloWorld">
    <dependencies>
        <build>
            CommonFileCopyEx/CopyCommonDataFiles
        </build>
    </dependencies>
    <sourcefiles basedir="source/HelloWorld">
        <includes name="**/*.cpp"/>
    </sourcefiles>
</Program>

<Program name="TestProgram2">
    <dependencies>
        <build>
            CommonFileCopyEx/CopyCommonDataFiles
        </build>
    </dependencies>
    <sourcefiles basedir="source/TestProgram2">
        <includes name="**/*.cpp"/>
    </sourcefiles>
</Program>
```
