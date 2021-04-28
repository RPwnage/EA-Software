---
title: Pre/Post Build Steps
weight: 182
---

Sections in this topic describe pre and post build steps in Utility modules, definition options, usage and examples

<a name="Usage"></a>
## Usage ##

Pre and post build steps in Utility modules are defined exactly the same as in Native modules: [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) .


{{% panel theme = "info" header = "Note:" %}}
Pre-Link Steps are not available in Utility modules
{{% /panel %}}
<a name="Example"></a>
## Example ##

Define  prebuild target, and use nant invocation to define prebuild command


```xml
.      <property name="runtime.buildmodules" value="TestExe"/>
          
.      <property name="runtime.TestExe.buildtype" value="Program"/>
          
.      <fileset name="runtime.TestExe.sourcefiles">
.        <includes name="source\*.cpp"/>
.      </fileset>
          
.      <fileset name="runtime.TestExe.objects">
.        <includes name="${nant.project.buildroot}\${myname}\${myver}\${config}\TestObj\CppTestGetInt.cpp.obj" asis="true"/>
.      </fileset>
          
.      <property name="runtime.TestExe.vcproj.pre-build-step">
.        ${nant.location}\nant.exe -buildfile:${package.dir}\${package.name}.build -D:config=${config} -masterconfigfile:${nant.project.masterconfigfile} -buildroot:${nant.project.buildroot} runtime.TestExe.prebuildtarget
.      </property>
          
.      <target name="runtime.TestExe.prebuildtarget">

.        <cc outputdir="${nant.project.buildroot}\${myname}\${myver}\${config}\TestObj">
.          <options>
.            -c
.          </options>
.          <sources basedir="${package.dir}\DummyObj">
.            <includes name="CppTestGetInt.cpp"/>
.          </sources>
.        </cc>

.      </target>
```
Define prebuild command


```xml
.      <property name="runtime.MyModule.buildtype" value="Utility" />
          
.      <property name="runtime.MyModule.vcproj.pre-build-step">
.        echo Executing  'MyModule' pre-build-step
.
.      </property>
.      <property name="runtime.MyModule.vcproj.post-build-step">
.          echo Executing  'MyModule' post-build-step
.      </property>
```

##### Related Topics: #####
-  [Pre/Post Build Steps in Native modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) 
