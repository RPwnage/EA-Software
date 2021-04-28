---
title: Custom Build Steps on Individual Files
weight: 107
---

Sections in this topic describe custom build files step usage and examples

<a name="CustomBuildFiles"></a>
## Custom Build Steps On Individual Files ##

Custom build files allow to attach a command with input and output dependencies to a file or collection of files.
Custom build files are similar to the [custom build step]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custombuildsteps.md" >}} ) but there can be many custom build files with different command attached to each file.

Another difference is in execution order, custom build steps on individual files are invoked right after [Prebuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) step.

To define `Custom Build Steps On Individual Files` in the package build script define the following structured XML fileset for a DotNet or Native module:

###### Define Custom Build Files In Structured XML ######

```xml
{{%include filename="/reference/packages/build-scripts/native-c/c++-modules/custom-build-files/custombuildfilesstructured.xml" /%}}

```
Alternatively to use old style syntax define the fileset:

 - {{< url groupname >}} `.custombuildfiles`  - {{< url fileset >}}that contains files with custombuildsteps,{{< url optionset >}}with cutom build step definitions must be attached to the files.

<a name="CustomBuildFileOptionset"></a>
## Custom Build File Optionset ##

To describe custom build command associated with files use optionset described below.
If custom build step is used in many packages in a game build, this optionset can be defined in a game configuration package.
Optionset can also be defined directly in a build script.

Optionset definition

option |required |Description |
--- |--- |--- |
| build.tasks | ![requirements 1a]( requirements1a.gif ) | List of logical command names(tasks). For each name(task) separate command can be defined. |
| [task].command | Either &#39;command&#39; or &#39;executable&#39; must be defined. | Command that is executed |
| [task].executable | Either &#39;command&#39; or &#39;executable&#39; must be defined. | Instead of defining full command line it is possible to define executable name and options |
| [task].options |  | Instead of defining full command line it is possible to define executable name and options |
| description |  | Text that will be printed before executing this step. Default: empty string. |
| name | ![requirements 1a]( requirements1a.gif ) | Name of this tool. The name used internally by Framework. Name is also printed instead of description when description is not defined. |
| outputs |  | List of output files. Each file on a separate line. |
| createoutputdirs |  | If set to  **true**  directories will be created for each file listed in &#39;outputs&#39;. Default: false |
| treatoutputascontent |  | If set to **true** &#39;TreatOutputAsContent&#39; element will be set in Visual Studio project for this Custom Build Step.<br>Default: false. |
| inputs |  | List of additional input files. Each file on a separate line. |
| input-filesets |  | The same of inputs but using a fileset. Value of this option should contain the name of a fileset. |
| sourcefiles |  | List of files. Each file on a separate line. |
| bulkbuild |  | If set to **true** files specified in &#39;sourcefiles&#39; will be added to [builkbuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) fileset as well (if builkbuild is enabled),<br>otherwise these files will be built as loose files. Default: false. |
| headerfiles |  | List of files. Each file on a separate line. |
| objectfiles |  | List of files. Each file on a separate line. |
| preprocess |  | Name of preprocess task or target. Preprocess is executed just before Framework starts to extract data<br>for this step and create instance of the BuildTool object. |
| postprocess |  | Name of postprocess task or target. Postproces is executed right after Framework extracted data<br>for this step and created instance of the BuildTool object. |

<a name="TemplateParameters"></a>
## Template Parameters ##

Option values in the optionset can contain template parameters. For each file in the `custombuildfiles`  optionset template parameters are substituted using this file info:


Template |Description |
--- |--- |
|  `%filepath%`  | Full path of the custombuildfile |
|  `%filename%`  | File name without extension for the custombuild file |
|  `%fileext%`  | File name extension for the custombuild file |
|  `%filedir%`  | Directory part of the path for the custombuildfile |
|  `%filebasedir%`  | Base directory for the custombuildfile. Base directory is a defined in the custombuildfiles {{< url fileset >}} |
|  `%filereldir%`  | custombuildfile path relative to the base directory |
|  `%intermediatedir%`  | Module intermediate build directory |
|  `%outputdir%`  | Module output build directory |

<a name="Example"></a>
## Example ##

Example of using custombuildfiles to compile files with custom compiler and add objectfiles to the library

###### Example Build Script ######

```xml
{{%include filename="/reference/packages/build-scripts/native-c/c++-modules/custom-build-files/custombuildcompile.xml" /%}}

```
Example of using custombuildfiles to generate sourcefiles and pass generated files to compiler

###### Example Build Script ######

```xml
{{%include filename="/reference/packages/build-scripts/native-c/c++-modules/custom-build-files/custombuildgenerate.xml" /%}}

```

##### Related Topics: #####
-  [Custom Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custombuildsteps.md" >}} ) 
-  [See BuildSteps example]( {{< ref "reference/examples.md" >}} ) 
