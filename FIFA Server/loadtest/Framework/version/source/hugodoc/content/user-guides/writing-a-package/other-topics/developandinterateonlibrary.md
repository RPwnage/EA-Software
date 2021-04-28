---
title: How to Develop and Iterate on a Library
weight: 22
---

This page provides a quick guide on how to develop and iterate on a library you created.

<a name="DefineModule"></a>
## Define a library module ##

Let&#39;s create a library module &#39;MyLibA&#39; for a package named &#39;MyLibraryPackage&#39;.
```xml
<project default="build" xmlns="schemas/ea/framework3.xsd">

    <!--
        This 'package' task will tell Framework to load the 'config' package and setup appropriate setting
        for the configuration that you're building.
    -->
    <package name="MyLibraryPackage" />

    <runtime>
        <Library name="MyLibA">
            <!-- The following setup the include path for the compiler switch. -->
            <includedirs>
                ${package.MyLibraryPackage.dir}/include
            </includedirs>
            <!-- The following setup the header files to be listed in Visual Studio project. -->
            <headerfiles basedir="${package.MyLibraryPackage.dir}/include">
                <includes name="*.h">
            </headerfiles>
            <!-- The following setup which native C++ source files to build. -->
            <sourcefiles basedir="${package.MyLibraryPackage.dir}/source/MyLibA">
                <includes name="*.cpp">
            </sourcefiles>
        </Library>
    </runtime>

</project>
```


The &lt;runtime&gt; element above indicates that this library module is in the runtime group.  Wrapping your
&lt;Library&gt; element inside the &lt;runtime&gt; group is actualy optional.  If you don&#39;t, by default, it will be
considered runtime group by Framework.

The &lt;Library&gt; XML element above specified a module named &#39;MyLibA&#39; with a &#39;Library&#39; build type.  For a full list of
other possible build types (such as DynamicLibrary), please consult the page [buildtype and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) for a full list or the page [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) to create your own.

<a name="ExportLibraryData"></a>
## Export library include paths and library paths in module public data ##

In order for other packages to use your library, you need to provide export information about your package.
Usually, they are just simply the include path and the library path.  We set up these information in a file
called Initialize.xml.  This is analogous to setting up your header file in your C++ module.

Let&#39;s setup your Initialize.xml as follows:
```xml
<project xmlns="schemas/ea/framework3.xsd">

    <publicdata packagename="MyLibraryPackage">

        <runtime>

            <!-- Declare export data for the C++ library module -->
            <module name="MyLibA">
                <includedirs>
                    ${package.MyLibraryPackage.dir}/include
                </includedirs>
                <libs>
                    <includes name="${package.MyLibraryPackage.builddir}/${config}/lib/${lib-prefix}MyLibA${lib-suffix}"/>
                </libs>
                <!-- Note that if this module is a DynamicLibrary module which creates dll, you need to export the following as well
                <dlls>
                    <includes name="${package.MyLibraryPackage.builddir}/${config}/bin/MyLibA${dll-suffix}"/>
                </dlls>
                -->
            </module>

            <!-- Alternatively, by specifying the 'buildtype' information, you can let Framework automatically create the necessary export fileset for you.
            <module name="MyLibA" buildtype="Library">
                <includedirs>
                    ${package.MyLibraryPackage.dir}/include
                </includedirs>
            </module>
            -->

        </runtime>

    </publicdata>

</project>
```


In the above example, we are exporting the path of the include folder and the path of the resulting built library.
Use the property &#39;package.[package_name].dir&#39; to specify the location of your package so that you don&#39;t need to
worry about the fact that your package can be installed to different location by different user.

The default output location of the built library is located in folder [build_root]\[YourPackageName]\[version]\[config]\lib.
You should use the property &#39;package.[package_name].builddir&#39; to specify [build_root]\[YourPackageName]\[version].  This way,
you don&#39;t need to worry about the fact that other user can have different [build_root] location.  The ${config} is the current
configuration user is building for.

Naming of the output file can be different for different platform.  Some with prefix and some with different file extension.
To allow the script to be generic across all platforms, you should use the &#39;lib-prefix&#39; and &#39;lib-suffix&#39; properties around your
module name.  So the above example ${lib-prefix}MyLibA${lib-suffix} will be resolved to &#39;libMyLibA.a&#39; on a Linux based platform,
while on PC, it will be resolved to &#39;MyLibA.lib&#39;.

<a name="DefineTestModule"></a>
## Define a test program module ##

After you finished creating your library, you should also create a test program to do a simple test use of your library.
Framework provided support for you to create modules that is destinated as testing. You just need to specify your program
module under &lt;tests&gt; group.  So in the previous .build file, you add the followings inside the &lt;project&gt; element.
```xml
<project default="build" xmlns="schemas/ea/framework3.xsd">
    . . .
    <tests>
        <Program name="MyTestProgram">

            <dependencies>
                <build>
                    MyLibraryPackage/runtime/MyLibA
                </build>
            </dependencies>

            <!-- The following setup which native C++ source files to your test program to build. -->
            <sourcefiles basedir="${package.MyLibraryPackage.dir}/tests/MyTestProgram">
                <includes name="*.cpp">
            </sourcefiles>

        </Program>
    </tests>

</project>
```


The above scripts added a program module &#39;MyTestProgram&#39; under the &lt;tests&gt; group.  In this module we introduced a new
element called &lt;dependencies&gt; to specify that we need to build and use the &#39;MyLibA&#39; library module.  The syntax is:
[Package_name]/[group]/[module_name].  However, if the [group] is runtime, we can omit that field.  So the above line
can be written as &#39;MyLibraryPackage/MyLibA&#39;.

<a name="BuildAndRunTest"></a>
## Build and run test program ##

To do a test build, you can either use &#39;nant&#39; command line build or a solution build.For &#39;nant&#39; build, go to a command prompt, then go to the package folder.  Type the followings:


```
nant.exe -configs:pc64-vc-dev-debug test-build
```
(assuming that you have nant.exe in your path and you have properly setup the masterconfig.xml in the package folder).
This &#39;test-build&#39; target will execute the build of the library and then a build of the test module program for the
configuration that is specified.

Note that if you just want to build the library and not the test, just run with &#39;build&#39; target.

For Visual Studio solution build, go to a command prompt, then go to the package folder.  Type the followings:


```
nant.exe -configs:"pc64-vc-dev-debug pc64-vc-dev-opt" slntest
```
(with the same assumption above).  This &#39;slntest&#39; target will create a Visual Studio solution with a project
for the library and another project for the test module and with configurations &quot;pc64-vc-dev-debug&quot; and &quot;pc64-vc-dev-opt&quot;
(the parameters specified in the -D:package.configs= settings).  Then to do a build on command line, type:


```
nant.exe -configs:pc64-vc-dev-debug visualstudio-test
```
Alternatively, you can also just open the solution and build from Visual Studio IDE.  Note that if you just want
to build the library and not the test, just use slnruntime and visualstudio targets.



After the test is built, to do a test run, you can execute the following nant command:


```
nant.exe -configs:pc64-vc-dev-debug test-run-fast
```
<a name="DependFromOtherPackage"></a>
## How other package depend on my library ##

When other package depends on your library, you need to make sure that other users added your package to the masterconfig file.  Then
all other packages needed to do is to add a dependency block to their package build script&#39;s module definition (vary similar to the
test module listed above):
```xml
<project default="build" xmlns="schemas/ea/framework3.xsd">
    . . .
    <package name="MyUserPackage" />

    <Program name="MyUserProgram">

        <dependencies>
            <build>
                MyLibraryPackage
            </build>
        </dependencies>

        <sourcefiles basedir="${package.MyUserPackage.dir}/source">
            <includes name="*.cpp">
        </sourcefiles>
    </Program>

</project>
```
In the above example, we make a build dependency to the entire package &#39;MyLibraryPackage&#39;.  This means that if MyLibraryPackage has
multiple modules, all modules will be added to your build dependency list.  However, if your user wish to constrain their build
dependent to a specific module, they can specify the block like so:
```xml
<project default="build" xmlns="schemas/ea/framework3.xsd">
    . . .
        <dependencies>
            <build>
                MyLibraryPackage/runtime/MyLibA
            </build>
        </dependencies>
    . . .
</project>
```


