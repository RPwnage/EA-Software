---
title: Use sln-taskdef target
weight: 30
---

Use “sln-taskdef” target to develop C# tasks

<a name="BPSlnTaskdef"></a>
## sln-taskdef target ##

sln-taskdef target will create Visual Studio solution to develop/debug NAnt C# tasks.
The task source file has to be added to the `<taskdef>` task to be include in generated solution.


```

  D:\packages\EAGimex\dev>nant -configs:pc-vc-dev-debug sln-taskdef
            [package] EAGimex-dev (pc-vc-dev-debug)  sln-taskdef
            [create-build-graph] runtime [Packages 14, modules 12, active modules 12]  (214 millisec)
        [visualstudio]     Updating project file  'vc-options.csproj'
        [visualstudio]     Updating project file  'script-init-verify.csproj'
        [visualstudio]     Updating project file  'eaconfig_tasks_TargetInit.csproj'
        [visualstudio]     Updating project file  'all-platforms-process.csproj'
        [visualstudio]     Updating solution file 'TaskDefCodeSolution.sln'
            [taskdef-sln-generator]   (222 millisec) generated files:
                  "D:\packages\EAGimex\dev\build\framework_tmp\TaskDefVisualStudioSolution\TaskDefCodeSolution.sln"
             [nant] BUILD SUCCEEDED (00:00:00)

```
