---
title: 4 - Depending On Other Packages
weight: 18
---
<a name="DependingOnOtherPackages"></a>

This pages builds upon  [the examples here]( {{< ref "user-guides/writing-a-package/dependingonotherpackages.md" >}} ) . It takes the core functionality
from that example and moves it to a separate module in a separate package. To create the new package a folder with the name of the new
package is needed and in this example the package files are going to be located in a sub-folder. Nant allows package contents to be nested
one level under the package name folder with the inner folder being used in indicate the version of the package. Versioning packages in this
way allows multiple versions to be easily kept side by side.

Package layout:


```
\HelloWorldLibrary
\1.00.00
 \include
  \HelloWorldLibrary
   say_hello.h
 \source
  say_hello.cpp
 HelloWorldLibrary.build
 Initialize.xml
 Manifest.xml
```
The build file is below. The &lt;includedirs&gt; element specifies which include directories should be used when compiling files in this
module.


```
<project xmlns="schemas/ea/framework3.xsd">
  <package name="HelloWorldLibrary"/>

  <Library name="Hello">
    <includedirs>
      include
    </includedirs>
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
  </Library>
</project>
```
In addition to the .build file, which describes how to build the library module, we also need to create an initialize.xml. This file is
used by Nant to expose information to *other* packages about modules in this package. In the below example there are
two important things to note.

 - The  *buildtype* attribute of the module matches the buildtype in the .build file. Declaring this<br>simplifies a lot the publicdata module definition by automatically exporting data about locations for output files of the module such as<br>libraries or assemblies.
 - The &lt;includedirs&gt; element exposes what include directories should be used by modules that depend on this one. In this<br>example this list is the same as the one in the .build file but it does not need to be. For example you may wish to separate internal and<br>external headers for your module.


```
<project xmlns="schemas/ea/framework3.xsd">
  <publicdata packagename="HelloWorldLibrary" >
      <module name="Hello" buildtype="Library">
        <includedirs>
          ${package.HelloWorldLibrary.dir}/include
        </includedirs>
      </module>
  </publicdata>
</project>
```
By taking the HelloWorldProgram from  [here]( {{< ref "user-guides/writing-a-package/dependingonotherpackages.md" >}} ) we can create a copy called
HelloWorldProgramWithDependencies and modify the .build file to look like this:

```
<project xmlns="schemas/ea/framework3.xsd">
  <package name="HelloWorldProgramWithDependencies"/>

  <Program name="SayHello">
    <dependencies>
      <auto>
        HelloWorldLibrary/Hello
      </auto>
    </dependencies>
    <sourcefiles>
      <includes name="source/*.cpp" />
    </sourcefiles>
  </Program>
</project>
```
The &lt;dependencies&gt; section allows the modules that this module depends on to be declared under a particular dependency type. In this
example the &lt;auto&gt; dependency indicates that the SayHello program needs the includes from Hello library as well as needing to link with it.
A far more comprehensive documentation page on dependencies can be found [here]( {{< ref "reference/packages/build-scripts/dependencies/_index.md" >}} ).

In order to use a dependency from another package however we need to specify the version of the package we want to use. Versions are always
controlled globally from the masterconfig file so need to add some additional information to the masterconfig:

```
<project xmlns="schemas/ea/framework3.xsd">
  <masterversions>
    <package name="WindowsSDK" 		version="10.0.16299" />
	
 <package name="HelloWorldLibrary"	version="1.00.00" />
  </masterversions>
  
  <packageroots>
 <packageroot>..\..</packageroot>
  </packageroots>

  <ondemand>true</ondemand>
  <ondemandroot>${nant.drive}/packages</ondemandroot>
  
  <buildroot>build</buildroot>
</project>
```
The two elements to take note of here are the &lt;package&gt; entry for HelloWorldLibrary and the &lt;packageroots&gt; section.

The package entry indicates the version of the HelloWorldLibrary to use. HelloWorldLibrary doesn&#39;t declare it&#39;s version in it&#39;s
Manifest.xml so nant will interpret it&#39;s version as the sub folder in which it lives under the folder with package name - in this example:
1.00.00.

The packageroots section tells nant which paths to search for packages when resolving dependencies before falling back to ondemand
sources. Since the HelloWorldLibrary only exists locally for now a search path adjacent to the HelloWorldProgramWithDependencies package
has been specfied. Relative paths are always resolved relative to the masterconfig file location.

We can now build the SayHello program:


```
[doc-demo] C:\HelloWorldProgramWithDependencies\1.00.00>nant -configs:pc64-vc-dev-debug slnruntime visualstudio
          [package] HelloWorldProgramWithDependencies-1.00.00 (pc64-vc-dev-debug)  slnruntime visualstudio
          [load-package] Loading package dependencies of HelloWorldProgramWithDependencies...
          [package] HelloWorldLibrary-1.00.00 (pc64-vc-dev-debug)  load-package-generate
          [load-package] Finished Loading Packages (117 millisec)
          [create-build-graph] Creating Build Graph...
          [create-build-graph] Global postprocess 1 step(s) (0 millisec)
          [create-build-graph] Finished - (190 millisec) [Packages 5, modules 2, active modules 2]
          [solution-generation] Generating Solution(s)...
      [visualstudio]     Updating project file  'SayHello.vcxproj.filters'
      [visualstudio]     Updating project file  'SayHello.vcxproj'
      [visualstudio]     Updating project file  'Hello.vcxproj.filters'
      [visualstudio]     Updating project file  'Hello.vcxproj'
      [visualstudio]     Updating solution file  'HelloWorldProgramWithDependencies.sln'
          [solution-generation] Finished (357 millisec) [C:\HelloWorldProgramWithDependencies\1.00.00\build\HelloWorldProgramWithDependencies\1.00.00\HelloWorldProgramWithDependencies.sln]
          [exec] C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE\devenv.com  /safemode /build "pc64-vc-dev-debug|x64" "C:\HelloWorldProgramWithDependencies\1.00.00\build\HelloWorldProgramWithDependencies\1.00.00\HelloWorldProgramWithDependencies.sln"

      Microsoft Visual Studio 2017 Version 15.0.27428.2002.
      Copyright (C) Microsoft Corp. All rights reserved.
      1>------ Build started: Project: Hello, Configuration: pc64-vc-dev-debug x64 ------
      1>say_hello.cpp
      1>Hello.vcxproj -> C:\HelloWorldProgramWithDependencies\1.00.00\build\HelloWorldLibrary\1.00.00\pc64-vc-dev-debug\lib\Hello.lib
      2>------ Build started: Project: SayHello, Configuration: pc64-vc-dev-debug x64 ------
      2>say_hello.cpp
      2>SayHello.vcxproj -> C:\HelloWorldProgramWithDependencies\1.00.00\build\HelloWorldProgramWithDependencies\1.00.00\pc64-vc-dev-debug\bin\SayHello.exe
      ========== Build: 2 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
           [nant] BUILD SUCCEEDED (00:00:10)

[doc-demo] C:\HelloWorldProgramWithDependencies\1.00.00>
```
