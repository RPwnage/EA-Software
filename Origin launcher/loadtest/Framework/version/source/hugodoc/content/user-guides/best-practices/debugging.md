---
title: Debugging
weight: 29
---

Techniques to debug build scripts

<a name="Section1"></a>
## Optional section title ##

Debugging nant and C# tasks is described here  [Debugging Framework]( {{< ref "user-guides/debugframework.md" >}} ) . In addition, there are several Framework features that can simplify debugging.

<a name="PBDebuggingBuildInfo"></a>
### Use  &quot;buildinfo&quot; target: ###

 [buildinfo]( {{< ref "reference/targets/buildinfo.md" >}} ) target outputs options, dependencies, buld steps and other info into a text file for every module.


```

D:\packages\EAGimex\dev>nant -configs:pc-vc-dev-debug buildinfo
    [package] EAGimex-dev (pc-vc-dev-debug)  buildinfo
    [create-build-graph] runtime [Packages 14, modules 12, active modules 12]  (212 millisec)
    [WriteBuildInfo]   (56 millisec) buildinfo directory : "D:\packages\EAGimex\dev\build\buildinfo\txt"
    [nant] BUILD SUCCEEDED (00:00:00)

```
File EAGimex-dev-runtime.EAGimex.txt contains complete info Framework has for the EAGimex module in given configuration

<a name="PBDebuggingTracePropo"></a>
### Nant –traceprop: option ###

Nant command line option  *-traceprop:* lets user to see any change to a nant property value


```

EAThread\dev>nant slntest -traceprop:runtime.EAThread.usedependencies

    [package] EAThread-dev (pc-vc-dev-debug-opt)  slntest
    TRACE set property runtime.EAThread.usedependencies=
    EABase
    at D:\packages\EAThread\dev\EAThread.build
    [package] EATest-dev (pc-vc-dev-debug-opt)  load-package-generate

```
<a name="PBDebuggingWarnLevel"></a>
### Use –warnlevel during development ###

 - **** Quiet - No warnings
 - **Minimal** - No most common warnings (package roots, duplicate packages)
 - **Normal** - Warnings on conditions that can affect results
 - **Advise** - Warnings on inconsistencies  in build scripts from which framework can auto recover.
 - **Deprecation** - Deprecation warnings
 - **Diagnostic** - Other warnings useful for diagnostic

 **-warnlevel:advise** - will output results of build scripts analysis done by Framework


```

D:\packages\EAThread\dev-kettle>nant slntest -warnlevel:advise
          [package] EAThread-dev-kettle (capilano-vc-dev-debug-opt)  slntest
      [warning] [EAThread-dev-kettle]: The value for 'test-build' in 'config.targetoverrides' is the same as the eaconfig default and thus can be removed!
      . . . . .
      [warning] module [EAThread_Test] has a build dependency on Package [EABase], but that package has no buildable modules for subsystem [<none>]
      [warning] Adding [EABase] as if it was an interface dependency.  The build scripts in Package [EAThread] should be changed to reflect this.
      . . . . .
      [warning] Attempting to generate a bulk-build file [BB_runtime.EATest.cpp] with only one include.  Reverting to a loose file. Consider changing the build script for package [EATest]
      . . . . .
      [warning]     [create-build-graph] Skipped transitive propagation to 'runtime.MemoryMan' via 'runtime.EASTL' of package 'EABase' (or one of its modules). Looks like module 'runtime.EASTL' declares use dependency on buildable package 'EABase'. Use dependency implies linking but package 'EABase is not built because no build dependency was defined. Correct build scripts to use interfacedependency (or builddependency) instead of usedependency

```
