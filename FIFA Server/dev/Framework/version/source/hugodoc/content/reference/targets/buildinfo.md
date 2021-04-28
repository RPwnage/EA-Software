---
title: buildinfo
weight: 210
---

 ** `buildinfo` ** target generates text files with detailed description for every module

<a name="buildinfo"></a>
## buildinfo target ##

When debugging problems in builds sometimes it is helpful to have information about every module settings, files and dependencies
in easy to read format, because extracting these data from log files or generated Visual Studio Solutions may not be easy.

#### Generate text files with detailed module descriptions ####
Target |Standard |Short description |
--- |--- |--- |
| buildinfo | ![requirements 1a]( requirements1a.gif ) | Text files in a folder ** `buildinfo` ** under the build root. |


{{% panel theme = "info" header = "Note:" %}}
buildinfo target is multiconfig target, use property [package.configs]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} ) to define list of configurations you are interested in.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
There are additional targets for building modules from specific groups: test-buildinfo, tool-buildinfo, and example-buildinfo.
To build all groups at once use: buildinfo-all.
{{% /panel %}}
<a name="Example"></a>
## Example ##

Here is example of using buildinfo target


```
D:\packages\EAAssert\1.03.02>nant buildinfo -D:package-configs=pc-vc-dev-debug
[package] EAAssert-1.03.02 (pc-vc-dev-debug)  buildinfo
[create-build-graph] runtime [Packages 5, modules 2, active modules 2]  (398 millisec)
[WriteBuildInfo]   (52 millisec) buildinfo directory : "D:\packages\EAAssert\1.03.02\build\buildinfo\txt"
[nant] BUILD SUCCEEDED (00:00:01)
```
File `D:\packages\EAAssert\1.03.02\build\buildinfo\txt\pc-vc-dev-debug\EAAssert-1.03.02-runtime.EAAssert.txt` contains detailed info about EAAssert module:


```
{{%include filename="/reference/targets/buildinfo/eaassert-1.03.02-runtime.eaassert.txt" /%}}

```
