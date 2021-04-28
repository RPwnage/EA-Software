---
title: 3 - Adding A Module To Package
weight: 17
---
<a name="AddingAModule"></a>

 [Modules]( {{< ref "reference/packages/modules.md" >}} ) are the common units of code defined in Framework and typically define how to build a single output such as a library, assembly,
DLL or executable. An example definition of a &quot;hello world&quot; executable module is below:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  <package name="HelloWorldProgram"/>

  <Program name="SayHello">
    <sourcefiles>
      <includes name="source/hello.cpp" />
    </sourcefiles>
  </Program>
</project>
```
Source files are not shown here but the Documentation/Examples folder of the Framework packages has working examples.

Many aspects of the module such as it&#39;s compiler and link options, output directory and so on can be further customized but eaconfig
and Framework provide a comprehensive set of default in order to keep module definitions terse.

In order to actually build the module we need to give nant a  [target]( {{< ref "reference/targets/_index.md" >}} ) to run on the command line. In this example we&#39;re using the *build*  target:


```
[doc-demo] C:\HelloWorldProgram\1.00.00>nant -configs:pc64-vc-dev-debug build
          [package] HelloWorldProgram-1.00.00 (pc64-vc-dev-debug)  build
          [load-package] Loading package dependencies of HelloWorldProgram...
          [load-package] Finished Loading Packages (37 millisec)
          [create-build-graph] Creating Build Graph...
          [create-build-graph] Global postprocess 1 step(s) (0 millisec)
          [create-build-graph] Finished - (182 millisec) [Packages 4, modules 1, active modules 1]
          [nant-build] building HelloWorldProgram-1.00.00 (pc64-vc-dev-debug) 'runtime.SayHello'
          [cc] C:\HelloWorldProgram\1.00.00\source\say_hello.cpp
          [link] SayHello
          [nant-build] finished target 'build' (1 modules) (614 millisec)
           [nant] BUILD SUCCEEDED (00:00:01)
           [nant] NOTE: This is a DEBUG build of NAnt and above time may be slower than an optimized build!

[doc-demo] C:\HelloWorldProgram\1.00.00>build\HelloWorldProgram\1.00.00\pc64-vc-dev-debug\bin\SayHello.exe
hello world

[doc-demo] C:\HelloWorldProgram\1.00.00>
```
The  *build* target tells nant to build the package directly by invoking the compiler / linker / etc. This
method of building isn&#39;t much used in modern nant however. Typically users prefer to generate and build Visual Studio solutions or XCode
projects. A Visual Studio solution build can be done with the *slnruntime* (generate solution) and *visualstudio* (build solution) targets. Default target reference can be found [here]( {{< ref "reference/targets/targetreference.md" >}} ) but any number of targets can be defined. These targets can be chained
together on the nant command line:


```
[doc-demo] C:\HelloWorldProgram\1.00.00>nant -configs:pc64-vc-dev-debug slnruntime visualstudio
          [package] HelloWorldProgram-1.00.00 (pc64-vc-dev-debug)  slnruntime visualstudio
          [load-package] Loading package dependencies of HelloWorldProgram...
          [load-package] Finished Loading Packages (36 millisec)
          [create-build-graph] Creating Build Graph...
          [create-build-graph] Global postprocess 1 step(s) (0 millisec)
          [create-build-graph] Finished - (187 millisec) [Packages 4, modules 1, active modules 1]
          [solution-generation] Generating Solution(s)...
      [visualstudio]     Updating project file  'SayHello.vcxproj.filters'
      [visualstudio]     Updating project file  'SayHello.vcxproj'
      [visualstudio]     Updating solution file  'HelloWorldProgram.sln'
          [solution-generation] Finished (395 millisec) [C:\HelloWorldProgram\1.00.00\build\HelloWorldProgram\1.00.00\HelloWorldProgram.sln]
          [exec] C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE\devenv.com  /safemode /build "pc64-vc-dev-debug|x64" "C:\HelloWorldProgram\1.00.00\build\HelloWorldProgram\1.00.00\HelloWorldProgram.sln"

      Microsoft Visual Studio 2017 Version 15.0.27428.2002.
      Copyright (C) Microsoft Corp. All rights reserved.
      1>------ Build started: Project: SayHello, Configuration: pc64-vc-dev-debug x64 ------
      1>say_hello.cpp
      1>SayHello.vcxproj -> C:\HelloWorldProgram\1.00.00\build\HelloWorldProgram\1.00.00\pc64-vc-dev-debug\bin\SayHello.exe
      ========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
           [nant] BUILD SUCCEEDED (00:00:16)
           [nant] NOTE: This is a DEBUG build of NAnt and above time may be slower than an optimized build!

[doc-demo] C:\HelloWorldProgram\1.00.00>
```
This package and module define some code but it&#39;s not something that can be easily reused. The next step would be create a library for
reusable functionality and [depend on that package]( {{< ref "user-guides/writing-a-package/dependingonotherpackages.md" >}} ) .

