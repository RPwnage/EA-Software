---
title: Standard Framework Targets
weight: 209
---

This topic describes some of the standard Framework targets and the properties that can affect their execution. Note that any package can override these targets with its own definition so behaviour may not be as described here in that case. There are two things to understand when choosing a target: 

* Some targets act on a single configuration defined by the [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property and some targets that act on multiple configurations, controlled by the [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property.
* Targets often act on a specific groups: one of the four hardcoded groups in Framework (runtime, example, test, tool). When Framework acts on a specific group it will take all modules from the top level package (the package Framework was invoked on) from that group and then build a graph of dependencies to modules in other packages. Groups are not considered for dependencies, only direct dependency links are followed. For example, creating a Visual Studio solution for a package's test group will not included the tests of dependencies in the solution if your test only depends on the runtime of other packages.

Targets can be broken down into a few broad categories:

 * [**Generation Targets**]({{ ref #GenerationTargets }}) - The most commonly used targets in Framework. These generate input files suitable for another build system and, where appropriate, corresponding IDE.
 * [**Build Targets**]({{ ref #BuildTargets }}) - Build targets invoke build systems on generated files with the correct parameters. For example, 'buildsln'  target invokes MSBuild to build projects created by  the 'gensln'  target.
 * [**Native NAnt Targets**]({{ ref #NAntTargets }}) - Framework itself can also be used as the build system without generating an intermediate set of a files for another build system. However, this is not a common use case, not supported for all platforms and poorly maintained. It is strongly recommended you use a generation / build target pair appropriate to your target platforms(s) instead of these.
 * [**Additional Targets**]({{ ref #AdditionalTargets }}) - Framework includes a  few standard targets for convience operations or for debugging. 

<a name=** | GenerationTargets** | ></a>
## Generation Targets ##

<a name=** | VisualStudioSlnTargets** | ></a>
### Visual Studio / MSBuild ###

For all platforms that have a Visual Studio Integration (VSI) we support these target will generate the appropriate projects files. 
For platforms that we support building on Windows but don't have a Visual Studio integration the solution generation targets will still work but create a solution of make-style project that wraps make files produced by make generate targets (which are called implicitly).

| Target | Short Description |
| --- | --- |
| **gensln** | Alias for 'slngroupall'. |
| **slngroupall** | Generates multiple configuration Visual Studio solutions for all groups (runtime, example, test, tool). To only generate for a subset of the 4 groups set the property slngenerator.slngroupall.groups to a space delimited string of requried groups. Generates to the same location as 'slnruntime'. |
| **slnruntime** | Generates Visual Studio Solution for runtime group and all configurations specified by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **slnexample** | Generates Visual Studio Solution for example group and all configurations specified by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **slntest** | Generates Visual Studio Solution for the test group and all configurations specified by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **slntool** | Generates Visual Studio Solution for the tool group and all configurations specified by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **slngroupall-generate-single-config** | Generates single configuration [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) Visual Studio solutions for all groups (runtime, example, test, tool). |
| **slnruntime-generate-single-config** | Generates Visual Studio Solution containing single [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) configuration for the runtime group. |
| **slnexample-generate-single-config** | Generates Visual Studio Solution containing single [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) configuration for the example group. |
| **slntest-generate-single-config** | Generates Visual Studio Solution containing single [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) configuration for the test group. |
| **slntool-generate-single-config** | Visual Studio Solution containing single [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) configuration for the tool group. |

<a name=** | XcodeGenerationTargets** | ></a>
### Xcode ###

For platforms that have to built via Xcode we support Xcode project generation. XCode projects are generated for multiple configurations defined in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})property. 

| Target | Short Description |
| --- | --- |
| **projectize** | Generates an Xcode project for the runtime group. |
| **projectize-test** | Generates an Xcode project for the test group group. |
| **projectize-example** | Generates an Xcode project for the example group group. |
| **projectize-tool** | Generates an Xcode project for the tool group group. |

<a name=** |	MakeGenerationTargets** |></a>
### Make File ###

Make targets generate make files without intermediate Visual Studio solution generation. On Windows &#39;vsimake&#39; compatible make files are generated, on Unix/Linux gnumake compatible makefiles are generated.
Makefiles are generated for multiple configurations defined in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})property. 
To control number of parallel jobs spawned by make use property *eaconfig.make-numjobs*.
{{% panel theme="info" header="** Note:**" %}}
Direct makefile generation copies nant build target settings and execution which may differ in pre/post build step definitions from Visual Studio builds.
{{% /panel %}}
{{% panel theme="info" header="** Note:**" %}}
Python, C# and External Visual Studio modules are not supported.
{{% /panel %}}

| Target | Short Description |
| --- | --- |
| **make-gen** | Generate make files for runtime group. |
| **make-test-gen** | Generate make files for test group. |
| **make-example-gen** | Generate make files for example group. |
| **make-tool-gen** | Generate make files for tool group. |
| **make-gen-groupall** | Generate make files for all groups. |

<a name=** | BuildTargets** | ></a>
## Build Targets ##

<a name=** | MSBuildBuildTargets** | ></a>
### MSBuild ###

| Target | Short Description |
| --- | --- |
| **buildsln** | Alias for 'msbuild'. |
| **msbuild** |	Builds solution generated by 'slnruntime' or 'slngroupall' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-example** |	Build solution generated by 'slnexample' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).  |
| **msbuild-test** | Build solution generated by 'slntest' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-tool** | Build solution generated by 'slntool' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-rebuild** |	Rebuild solution generated by 'slnruntime' or 'slngroupall' target with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-rebuild-example** | Rebuild solution generated by 'slnexample' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-rebuild-test** | Rebuild solution generated by 'slntest' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-rebuild-tool** | Rebuild solution generated by 'slntool' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-clean** | Clean solution generated by 'slnruntime' or 'slngroupall' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-clean-example** | Clean solution generated by 'slnexample' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-clean-test** | Clean solution generated by 'slntest' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-clean-tool** | Clean solution generated by 'slntool' target using msbuild.exe with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **msbuild-all** | Build solution generated by 'slnruntime' or 'slngroupall' target using msbuild.exe for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **msbuild-all-example** | Build solution generated by 'slnexample' target using msbuild.exe for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **msbuild-all-test** | Build solution generated by 'slntest' target using msbuild.exe for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **msbuild-all-tool** | Build solution generated by 'slntool' target using msbuild.exe for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |

<a name=** | VisualStudioBuildTargets** | ></a>
### Visual Studio ###

{{% panel theme="info" header="** Note:**" %}}
Builds executed via Visual Studio (devenv.exe) are similar, but not identical, to builds via msbuild.exe. 
Visual Studio uses it's own project build scheduling and "fast" dependency check for each project, much like executing the build from within the Visual Studio GUI.
Additionally, some MSBuild logic is subtly different depending on whether the build is launched from Visual Studio or not.
While faster in small incremental cases, Visual Studio builds are more likely to have incremental state issues. It is recommended for automation / verification purposes to use MSBuild. 
Visual Studio should only be used for local iteration.
{{% /panel %}}

| Target | Short Description |
| --- | --- |
| **visualstudio** | Builds solution generated by 'slnruntime' or 'slngroupall' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).
| **visualstudio-example** | Build solution generated by 'slnexample' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).  |
| **visualstudio-test** | Build solution generated by 'slntest' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **visualstudio-tool** | Build solution generated by 'slntool' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **visualstudio-all** | Build solution generated by 'slnruntime' or 'slngroupall' target using Visual Studio for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **visualstudio-all-example** | Build solution generated by 'slnexample' target using Visual Studio for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **visualstudio-all-test** | Build solution generated by 'slntest' target using Visual Studio for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **visualstudio-all-tool** | Build solution generated by 'slntool' target using Visual Studio for every config in [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) property. |
| **visualstudio-clean** | Clean solution generated by 'slnruntime' or 'slngroupall' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).
| **visualstudio-example-clean** | Clean solution generated by 'slnexample' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).  |
| **visualstudio-test-clean** | Clean solution generated by 'slntest' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **visualstudio-tool-clean** | Clean solution generated by 'slntool' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **visualstudio-deploy** | Execute deploy step for solution generated by 'slnruntime' or 'slngroupall' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).
| **visualstudio-example-deploy** | Execute deploy step for solution generated by 'slnexample' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}).  |
| **visualstudio-test-deploy** | Execute deploy step for solution generated by 'slntest' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **visualstudio-tool-deploy** | Execute deploy step for solution generated by 'slntool' target using Visual Studio with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |

#### Xcode ####
| Target | Short Description |
| --- | --- |
| **xcodebuild** | Builds Xcode project generated by 'projectize' target using Xcode with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **xcodebuild-example** | Builds Xcode project generated by 'projectize-example' target using Xcode with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **xcodebuild-test** | Builds Xcode project generated by 'projectize-test ' target using Xcode with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **xcodebuild-tool** | Builds Xcode project generated by 'projectize-tool' target using Xcode with specified [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |

#### Make File ####
| Target | Short Description |
| --- | --- |
| **make-build** | Builds configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-gen' target. |
| **make-test-build** | Builds configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-test-gen' target. |
| **make-example-build** | Builds configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-example-gen' target. |
| **make-tool-build** | Builds configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-tool-gen' target. |
| **make-buildall** | Builds all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-gen' target.|
| **make-test-buildall** | Builds all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-test-gen' target.|
| **make-example-buildall** | Builds all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-example-gen' target.|
| **make-tool-buildall** | Builds all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-tool-gen' target.|
| **make-clean** | Cleans configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-gen' target. |
| **make-test-clean** | Cleans configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-test-gen' target. |
| **make-example-clean** | Cleans configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-example-gen' target. |
| **make-tool-clean** | Cleans configuration *[config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}})* using nakefiles generated by 'make-tool-gen' target. |
| **make-cleanall** | Cleans all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-gen' target.|
| **make-test-cleanall** | Cleans all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-test-gen' target.|
| **make-example-cleanall** | Cleans all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-example-gen' target.|
| **make-tool-cleanall** | Cleans all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) using nakefiles generated by 'make-tool-gen' target.|

<a name=** | NAntTargets** | ></a>
## Native NAnt Targets ##

{{% panel theme="info" header="** Note:**" %}}
Native NAnt builds targets are an older feature that is not widely used, not supported for all platforms and poorly maintained. 
It is strongly recommended you use a generation / build target pair appropriate to your target platform(s) instead of these.
{{% /panel %}}

| Target | Short Description |
| --- | --- |
| **build** | Build runtime group using specifed [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **example-build** | Build example group using specifed [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **test-build** | Build test group using specifed [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **tool-build** | Build tool group using specifed [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **buildall** | Build runtime group in all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) |
| **example-buildall** | Build example group in all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) |
| **test-buildall** | Build test group in all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) |
| **tool-buildall** | Build tool group in all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) |
| **clean** | Cleans results of a build for specified( [config]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}). |
| **cleanall** | Cleans results of a build for all configurations defined by [package.configs]({{<ref "reference/properties/configurationproperties.md#ConfigProperties">}}) |

<a name=** | AdditionalTargets** | ></a>
## Additional Targets ##

| Target | Short Description |
| --- | --- |
| **visualstudio-open** | Opens generated Visual Studio solution for runtime group, created by 'slnruntime' or 'slngroupall'. |
| **visualstudio-example-open** | Opens generated Visual Studio solution for example group. |
| **visualstudio-test-open** | Opens generated Visual Studio solution for test group. |
| **visualstudio-tool-open** | Opens generated Visual Studio solution for tool group. |
| **buildinfo** | Outputs information from Framework's internal dependency graph. Useful for debugging or analysing your project's dependencies. See [buildinfo]({{<ref "reference/targets/buildinfo.md">}}) documentation for more information. |
