---
title: Custom Build Steps
weight: 106
---

Sections in this topic describe custom build steps step usage and examples

<a name="Usage"></a>
## Usage ##

Custom Build step is modeled after Visual Studio but is is supported in nant builds in Framework as well.
By default Custom Build is executed after link step and before [Post Build Step]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) .

However, it can be set to execute before or after some targets. (ie. prebuild, postbuild, prelink, prerun, postrun)There is one custom build step per {{< url Module >}}.
However, since both before and after can be set to different values, the same custom build step can be executed twice.
It is possible to define similar build steps per file, see [Custom Build Files]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custom-build-files.md" >}} ) 

The difference between Custom Build Step and [Post Build Step]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) is that Custom Build Step allows to define input and output dependencies, and is executed only when input dependencies are newer than outputs.

To define Custom Build Step specify following properties:

 - {{< url groupname >}} `.vcproj.custom-build-tool` - command to be executed by the custom build step
 - {{< url groupname >}} `.vcproj.custom-build-outputs` - property should contain list of files that are added to<br>the step output dependencies. Each file path should be on a separate line.
 - {{< url groupname >}} `.vcproj.custom-build-dependencies` - property should contain list of files that are added to<br>the step input dependencies. Each file path should be on a separate line.
 - fileset {{< url groupname >}} `.vcproj.custom-build-dependencies` - fileset with input dependencies.<br><br>This property is optional. If input dependencies are not specified step is executed when output(s) do not exist
 - {{< url groupname >}} `.custom-build-before-targets` - The name of a target that this custom build target<br>should run before. Currently native NAnt builds only support **Build** ,  **Link**  and  **Run** ,<br>however visual studio builds don&#39;t support **Run** and it is best to design your package to maintain consistency between visualstudio and nant builds.
 - {{< url groupname >}} `.custom-build-after-targets` - The name of a target that this custom build target<br>should run after. Currently native NAnt builds only support **Build**  and  **Run** ,<br>however visual studio builds don&#39;t support **Run** and it is best to design your package to maintain consistency between visualstudio and nant builds.

<a name="Example"></a>
## Example ##

Input dependencies definition is optional.


```xml
<property name="runtime.MyModule.buildtype" value="Program" />
    
<property name="runtime.MyModule.vcproj.custom-build-tool">
 copy ${package.dir}\${package.name}.build ${package.builddir}\MyModule_${config}.txt
</property>
          
<property name="runtime.MyModule.vcproj.custom-build-outputs">
 ${package.builddir}\MyModule_${config}.txt
</property>
   
<property name="runtime.MyModule.custom-build-after-targets" value="Run" />
```
Custom Build Step can be specified for  [Utility]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) modules.


```xml
<property name="runtime.MyModule.buildtype" value="Utility" />

<property name="outfile" value="${package.configbuilddir}/somefile.txt" />
          
<property name="runtime.MyModule.vcproj.custom-build-tool">
 copy @{PathToWindows('${package.dir}\${package.name}.build')} @{PathToWindows('${outfile}')}
</property>
<property name="runtime.MyModule.vcproj.custom-build-outputs">
 ${outfile}
</property>
<fileset name="runtime.MyModule.vcproj.custom-build-dependencies">
 <includes name="${package.dir}\${package.name}.build"/>
</fileset>
```
Here is an example of how to set custom build steps in Structured XML:


```xml
<CSharpProgram name="SecondModule">
  <sourcefiles basedir="${package.dir}/source">
    <includes name="${package.dir}/source/*.cs" />
  </sourcefiles>
  <buildsteps>
    <custom-build-step before="Run" after="Run">
      <custom-build-tool>
        echo "Structured Custom Build Event"
      </custom-build-tool>
    </custom-build-step>
  </buildsteps>
</CSharpProgram>
```

##### Related Topics: #####
-  [Custom Build Steps on Individual Files]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custom-build-files.md" >}} ) 
-  [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) 
-  [See BuildSteps example]( {{< ref "reference/examples.md" >}} ) 
