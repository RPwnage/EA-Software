---
title: Create Prebuilt Package
weight: 214
---

 **create-prebuilt-package** target helps people create a prebuild version of their package.
The created prebuilt package contains the bare minimal information (new package build file, built binaries,
include directories, and content files as specified in the export data in initialize.xml).

<a name="CreatePrebuiltPackage"></a>
## create-prebuilt-package target ##

This target helps you create a prebuilt version of your package so that you can ship this package to other developers
as a prebuilt library instead of exposing source code to external developers.

The creation of this prebuilt package requires the following basic assumptions:

 - You have already built the package with all the configs that you want to ship. Building is not part of the<br>prebuilt target because different configs require different build methods. Also, not rebuilding takes advantage<br>of using a build that has already been done instead of forcing another rebuild.
 - Any global property affecting the build of this prebuilt binary needs to be the same when you create this<br>prebuilt package. Once this prebuilt package is created and starts being used in place of original package<br>you cannot change build properties because your binaries are already built.

Once the above assumptions are satisfied, the operation of this target consists of the following steps to create a
new prebuilt package.  This new package will have a new version name with &quot;-prebuilt&quot; appended to the old version name.NOTE:If &#39;modify-in-place&#39; option is used, then instead of creating a new standalone prebuilt package, only the
built artifacts will be copied to output directory and the .build and Initialize.xml files will be modified in place.

 - A build graph is created based on the config set in &#39;package.configs&#39;.  It is also created for all<br>build groups (ie runtime, tool, test, and example).  If this is not desirable, you can set property<br>eaconfig.create-prebuilt.buildgroups to override the list.
 - Once the build graph for all configs are created, it loops through all the top modules (ie all modules<br>in current package) and creates a new build file. This new build file converts all existing module to<br>&quot;Utility&quot; modules (with dependency list preserved). The old build file will be ignored, so if you have<br>any custom targets that need to be shipped, you may want to put those in your initialize.xml.
 - The entire &#39;scripts&#39; folder (including the Initialize.xml) will be copied to the prebuilt package.<br>Then the original Initialize.xml will be renamed as Initialize-Original.xml in the prebuilt package.<br>Afterwards, a new Initialize.xml will be created. This new Initialize.xml will first include the<br>Initialize-Original.xml, then set up overrides for the export property &#39;includedirs&#39;, and export<br>filesets such as &#39;libs&#39;, &#39;dlls&#39;, and &#39;assemblies&#39;. Sometimes, your export &#39;includedirs&#39; may have been<br>setup to point to auto generated files in your ${package.builddir}. These paths will be converted<br>to ${package.dir}\GeneratedFiles and the appropriate content will be copied over as well. The new<br>fileset overrides will also convert all the ${package.builddir} paths to ${package.dir}\bin (for dlls and assemblies)<br>and ${package.dir}\lib (for libs).
 - The original package&#39;s Manifest.xml will be copied to the prebuilt package location and have the versionName field<br>updated (or added if missing) with the new version name.
 - Then the built binaries will be copied to the prebuilt package and placed under &#39;bin&#39; or &#39;lib&#39; folder as appropriate.
 - Then the &#39;includedirs&#39; listed for all modules (or just package level) in your original Initialize.xml will be copied<br>to the new prebuilt package.  Note that only files with .h, .hpp, and .inl files extension will be copied to the new<br>prebuilt package.  You can further control certain files to exclude by providing the fileset<br>&#39;package.&lt;PackageName&gt;.PrebuiltExcludeSourceFiles&#39; in your original Initialize.xml.
 - Then any files listed in &#39;contentfiles&#39; in your original Initialize.xml will be copied to your new prebuilt package.
 - Finally, if your package has any extra files that needs to be included in the final prebuilt-package be not listed in<br>the above standard fileset, you can create a fileset named &#39;package.&lt;PackageName&gt;.PrebuiltIncludeSourceFiles&#39;.  Any<br>files specified in this fileset will be copied over to the output directory as well.

#### Create prebuilt package ####
Target |Standard |Short description |
--- |--- |--- |
| create-prebuilt-package | ![requirements 1a]( requirements1a.gif ) | Convert your package to a prebuilt package |

The following properties can be used to configure the prebuilt package generation.

#### Create Prebuilt Package Configuration ####
Property |Default value |Description |
--- |--- |--- |
| eaconfig.create-prebuilt.buildgroups | runtime tool example test | Space separated list of build groups to be considered for the prebuilt package. |
| eaconfig.create-prebuilt.outputdir | ${nant.project.buildroot}\Prebuilt | Output location of the prebuilt package. |
| eaconfig.create-prebuilt.test-for-boolean-properties | [Empty string] | The new generated build file and Initialize.xml will add conditional attributes to test for indicated boolean properties. |
| eaconfig.create-prebuilt.extra-token-mappings | [Empty string] | In order for generated prebuilt package to be portable, the re-generated Initialize.xml cannot contain<br>hard coded path.  This task already attempted to re-tokenize the full path back into property name<br>tokens.  However, if you have other custom properties setup, you can supply the property names (separated by<br>semi-colon) and this task will attempt to re-tokenize the paths using these properties. |
| eaconfig.create-prebuilt.redirect-includedirs | false | Have the generated Initialize.xml redirect the includedirs property back to original source tree.<br>If you use this input, it is expected that you will also use &#39;extra-token-mappings&#39; option to allow<br>the tool to convert the source tree root to some property so that you won&#39;t end up with hard coded<br>full path in final includedirs property. |
| eaconfig.create-prebuilt.redirect-includedirs-for-rootdirs | [Empty string] | Similar to &#39;redirect-includedirs&#39; exception that this redirection will only occurs for path<br>starting with the indicated rootdirs. |

<a name="CreatePrebuiltPackageExample"></a>
## Example ##

Here is example of using this target in basic nant commands


```
# Note that slngroupall target is used to create the solution to allow inclusion of modules in all build groups.
> nant.exe -configs:"pc-vc-dev-debug pc-vc-dev-opt" slngroupall
> nant.exe -configs:pc-vc-dev-debug visualstudio
> nant.exe -configs:pc-vc-dev-opt visualstudio
...

> nant.exe -configs:"pc-vc-dev-debug pc-vc-dev-opt" create-prebuilt-package
[package] EAAssert-1.05.03 (pc-vc-dev-debug)  create-prebuilt-package
...
[create-build-graph] Global postprocess 1 step(s) (0 millisec)
[create-build-graph] runtime tool example test [Packages 22, modules 18, active modules 18]  (191 millisec)
[create-prebuilt-package] Extracting all module info in package EAAssert for the config list: pc-vc-dev-debug pc-vc-dev-opt
[create-prebuilt-package] Creating new prebuilt package at: D:\packages\EAAssert\1.05.03\build\Prebuilt\EAAssert\1.05.03-prebuilt
[create-prebuilt-package] Constructing a new build file ...
[create-prebuilt-package] Copying old Initialize.xml as Initialize-Original.xml and constructing a new initialize.xml file ...
[create-prebuilt-package] Copying Manifest.xml ...
[create-prebuilt-package] Copying built binaries (if any) ...
[create-prebuilt-package] Copying include directories - .h, .hpp, and .inl only (if any) ...
[create-prebuilt-package] Copying content files (if any) ...
[create-prebuilt-package] Prebuilt package successfully created at: D:\packages\EAAssert\1.05.03\build\Prebuilt\EAAssert\1.05.03-prebuilt
[nant] BUILD SUCCEEDED (00:00:01)
```
Here is another example under Frostbite fbcli environment:


```
> fb gensln win64
 ...
> fb buildsln win64 release
 ...
> fb buildsln win64 debug
 ...

> fb nant EAAssert "win64-vc-dll-debug win64-vc-dll-release" create-prebuilt-package
 ...
```
