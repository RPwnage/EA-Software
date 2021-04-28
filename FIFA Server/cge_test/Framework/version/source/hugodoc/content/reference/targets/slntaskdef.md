---
title: sln-taskdef target
weight: 211
---

sln-taskdef target creates Visual Studio solution for nant tasks built using{{< task "NAnt.Core.Tasks.TaskDefTask" >}}task.

<a name="sln-taskdef"></a>
## sln-taskdef ##

When developing NAnt C# tasks it is convenient to have Visual Studio Solution with task code and
all references to Framework assemblies set.

#### Generate Visual Studio Solution With NAnt tasks ####
Target |Standard |Short description |
--- |--- |--- |
| sln-taskdef | ![requirements 1a]( requirements1a.gif ) | Creates Visual Studio Solution with NAnt tasks<br>built using{{< task "NAnt.Core.Tasks.TaskDefTask" >}}task. |

<a name="Example"></a>
## Example ##

Here is example of using sln-taskdef target on EAMTestApp


```
nant.exe sln-taskdef -buildroot:buildroot -configs:pc-vc-dev-debug
[package] EAMTestApp-dev (pc-vc-dev-debug)  sln-taskdef
6>        [package] EAWebKitUtil-12.4.2.0.0 (pc-vc-dev-debug)  load-package-generate
4>        [package] EAMUTFWin-1.09.06 (pc-vc-dev-debug)  load-package-generate
21>       [GenerateBulkBuildFiles] "XHTML/XHTML": Updating 'BB_runtime.XHTML.cpp'
. . . . . . . . .
. . . . . . . . .

62>       [GenerateBulkBuildFiles] "rwmovie/rwmovie_snd9": Updating 'BB_runtime.rwmovie_snd9.cpp'
62>       [GenerateBulkBuildFiles] "rwmovie/rwmovie_vp6": Updating 'BB_runtime.rwmovie_vp6.cpp'
62>       [GenerateBulkBuildFiles] "rwmovie/rwmovie_vp8": Updating 'BB_runtime.rwmovie_vp8.cpp'
[create-build-graph] runtime [Packages 92, modules 121, active modules 121]  (944 millisec)
[visualstudio]     Updating project file  'vc-options.csproj'
[visualstudio]     Updating project file  'eaconfig_tasks_TargetInit.csproj'
[visualstudio]     Updating project file  'all-platforms-process.csproj'
[visualstudio]     Updating project file  'script-init-verify.csproj'
[visualstudio]     Updating project file  'eamconfig.csproj'
[visualstudio]     Updating solution file 'TaskDefCodeSolution.sln'
[taskdef-sln-generator]   (313 millisec) generated files:
"D:\packages\EAMTestApp\dev\buildroot\framework_tmp\TaskDefVisualStudioSolution\TaskDefCodeSolution.sln"
[nant] BUILD SUCCEEDED (00:00:03)
```
All NAnt tasks built using{{< task "NAnt.Core.Tasks.TaskDefTask" >}}are included in the solution.

Each `<taskdef>` will result in a separate project, projects are organized
in folders according to package names.

Tasks from eaconfig and EAMconfig are included in the solution![Sln Taskdef Solution]( slntaskdefsolution.png )
```
{{%include filename="/reference/targets/slntaskdef/eaassert-1.03.02-runtime.eaassert.txt" /%}}

```

##### Related Topics: #####
- {{< task "NAnt.Core.Tasks.TaskDefTask" >}}
