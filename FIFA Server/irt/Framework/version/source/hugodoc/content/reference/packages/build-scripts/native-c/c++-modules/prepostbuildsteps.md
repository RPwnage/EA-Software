---
title: Pre/Post Build Steps
weight: 105
---

Sections in this topic describe pre and post build step, definition options, usage and examples

<a name="Overview"></a>
## Overview ##

Pre and post build steps are executed during build phase whether it is nant build, Visual Studio Solution build, or Xcode build.

There are some slight differences in pre/post build steps between nant build and Visual Studio builds.
nant builds can execute targets or os commands, while Visual Studio can only execute OS commands.
nant builds will always execute the post build command, whereas Visual Studio will only execute the post build command if something is built.

{{% panel theme = "info" header = "Note:" %}}<br/>Standalone targets (without commands) are not recommended in the Visual Studio solution workflow. Targets without commands force an autogenerate of the target command which is costly for builds and may generate non-portable code. As such the ability to reference a target without an accompanying command will eventually be deprecated.<br/>{{% /panel %}}

Framework provides a way to specify pre/post build steps as targets, commands, or both.
Framework can also [automatically convert]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md#TargetToCommandAutoconversion" >}} ) target
to a command that invokes nant executable with specified target.
For backwards compatibility this conversion does not work by default, but can be turned on for particular targets.

Framework supports following build steps:

 - `prebuild` - executed before any other build action happens for a module build.
 - `prelink` - executed after compilation and before link step.
 - `postbuild` - executed after build is completed.

<a name="Usage"></a>
## Usage ##

### Defintions and Behavior ###

Pre/postbuild and prelink events can be defined as:

- **nant target**: Build steps defined as nant targets are executed during nant builds (_build_ target). Visual Studio project cannot execute nant target directly, it requires command or script. To use nant target it needs to be converted to a command of form: `nant target_name ...`.
 
{{% panel theme = "info" header = "Note:" %}}
<br/>
When using [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} ), command definition is automatically created from target if no command is specified explicitly.<br/>When using traditional syntax nant target is not converted to command definition automatically by default for bacwards compatibility with existing scripts. To turn on automatic command creation define property: {{< url groupname >}}.buildstepname.targetname.nantbuildonly = false, where `buildstepname` is name of the build step: `prebuild, postbuild, prelink`, and `targetname` is name of the nant target.
{{% /panel %}}
 
- **OS command/script** When command or script is defined for a build step it will be executed in both nant build and Visual Studio or Xcode build.

- **Both definitions as nant target and OS command/script for the same step** In this case nant target is used in nant builds and command is used in in Visual Studio builds.

For some tasks it&#39;s more convenient to define nant targets, for others command or script.


{{% panel theme = "info" header = "Note:" %}}
<br/>
To copy build outputs (usually dlls) from dependent packages use  **copylocal**  property instead of using  *copy* command in a postbuild step. Postbuildstep may not be invoked in incremental build if project is uptodate.
Framework adds targets and properties into generated Visual Studio solution to make sure that copylocal step is always executed. **copylocal**  can be turned on for alldependent modules (see  [copylocal for native modules]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/copylocal.md" >}} ) and [copylocal for C# modules]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/properties/copylocal.md" >}} ) ).
{{% /panel %}}
### Pre Build Step ###

Using  [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} )  prebuild step can be defined as

 - Prebuild step defined as a target, with inlined target definition:
 
###### prebuild step defined as nant target ######
 
 ```xml

      <Library name="Hello">
        . . . . .
        <buildsteps>
          <prebuild-step>
            <target>
              <echo>Executing prebuild nant target</echo>
            </target>
          </prebuild-step>
        </buildsteps>
      </Library>

```
 
 {{% panel theme = "info" header = "Note:" %}}
 <br/>
When generating Visual Studio solutions nant target is automatically converted to the nant.exe invocation that will execute given target.

build file, masterconfig, command line properties and global properties are all set on the generated command line.
{{% /panel %}}
 - Prebuild step references nant target defined elsewhere:

###### prebuild step defined as nant target ######

```xml

      <Library name="Hello">
        . . . . .
        <buildsteps>
          <prebuild-step targetname="example-prebuild-step-target"/>
        </buildsteps>
      </Library>
                    
                    
      <target name="example-prebuild-step-target">
        <echo>Executing "example-prebuild-step-target" prebuild nant target</echo>
      </target>

```
 - Prebuild step with command

###### prebuild step defined as a command/script ######

```xml

      <Library name="Hello">
        . . . . .
        <buildsteps>
          <prebuild-step>
            <command>
              @echo Module 'Hello': executing 'PreBuild Step', copy 'say_hello.txt' to 'say_hello.cpp'
              ${nant.copy_32} -otli ${package.dir}/data/say_hello.txt ${package.builddir}/source/hello/say_hello.cpp
            </command>
          </prebuild-step>
        </buildsteps>
      </Library>

```
 - Prebuild step can contain both target and command defintions:

###### prebuild step definition as target and as a command/script ######

```xml

      <Library name="Hello">
        . . . . .
        <buildsteps>
          <prebuild-step>
            <target>
              <echo>Executing prebuild nant target</echo>
            </target>
            <command>
              @echo Module 'Hello': executing 'PreBuild Step', copy 'say_hello.txt' to 'say_hello.cpp'
              ${nant.copy_32} -otli ${package.dir}/data/say_hello.txt ${package.builddir}/source/hello/say_hello.cpp
            </command>
          </prebuild-step>
        </buildsteps>
      </Library>

```
In this case nant target is executed in nant builds and command is executed in Visual Studio builds.

When using traditional syntax following targets or properties can be used to define prebuild target(s)

 - **eaconfig.prebuildtarget** - this property usually used by configuration packages to set up prebuild targets executed for every module.

If property `eaconfig.prebuildtarget`  does not exist Framework will execute target named  `eaconfig.prebuildtarget` if such target exists.
 - **eaconfig.${config-platform}.prebuildtarget** - this property usually used by configuration packages to set up prebuild targets executed
for every module for given `${config-platform}` 

If property `eaconfig.${config-platform}.prebuildtarget` does not exist Framework will
execute target named `eaconfig.${config-platform}.prebuildtarget` if such target exists.
 - option **prebuildtarget**  - name of the prebuild target(s) can be specified in the {{< url buildtype >}}optionset.
 - {{< url groupname >}} **.prebuildtarget** - name of the prebuild target(s) for given module.

If property `[group].[module].prebuildtarget` does not exist Framework will
execute target named `[group].[module].prebuildtarget` if such target exists.

Following targets or properties can be used to define prebuild command

 - **eaconfig.vcproj.pre-build-step** - this property usually used by configuration packages to set up prebuild commands executed for every module.
 - **eaconfig.${config-platform}.vcproj.pre-build-step** - this property usually used by configuration packages to set up prebuild commands executed
for every module for given `${config-platform}`
 - option **vcproj.pre-build-step**  - name of the prebuild command can be specified in the {{< url buildtype >}}optionset.
 - {{< url groupname >}} **.vcproj.pre-build-step** - name of the prebuild command for given module.


{{% panel theme = "info" header = "Note:" %}}
Comands can contain multiple lines, each line defines separate OS command
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
For DotNet modules prefix  `vcproj`  should be changed to  `csproj`  or  `fsproj`
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Targets and command are executed in order they are listed above.
{{% /panel %}}
### Pre Link Step ###

Pre link step definitions are the same as for pre-build step definitions except that  `prebuildtarget` should be substituted
with `prelinktarget` , and  `pre-build-step` should be substituted
with `pre-link-step` 

 - **eaconfig.prelinktarget**
 - **eaconfig.${config-platform}.prelinktarget**
 - option **prelinktarget**
 - {{< url groupname >}} **.prelinktarget**

Pre-link commands

 - **eaconfig.vcproj.pre-link-step**
 - **eaconfig.${config-platform}.vcproj.pre-link-step**
 - option **vcproj.pre-link-step**
 - {{< url groupname >}} **.vcproj.pre-link-step**

### Post Build Step ###

Post build step definitions are the same as for pre-build step definitions except that `prebuildtarget` should be substituted
with `postbuildtarget` , and  `pre-build-step` should be substituted
with `pre-link-step` 

Using [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} ) :

###### postbuild step definition as target and as a command/script ######

```xml

      <Library name="Hello">
        . . . . .
        <buildsteps>
          <postbuild-step>
            <target>
              <echo>Executing postbuild nant target</echo>
            </target>
            <command>
              @echo Executing postbuild command
            </command>
          </postbuild-step>
        </buildsteps>
      </Library>

```
Using traditional syntax:

 - **eaconfig.postbuildtarget**
 - **eaconfig.${config-platform}.postbuildtarget**
 - option **postbuildtarget**
 - {{< url groupname >}} **.postbuildtarget**

Post-build commands

 - **eaconfig.vcproj.post-build-step**
 - **eaconfig.${config-platform}.vcproj.post-build-step**
 - option **vcproj.post-build-step**
 - {{< url groupname >}} **.vcproj.post-build-step**

### Template parameters in commands ###

Command definitions can contain following template parameters that are substituted with corresponding values during build graph creation
between steps [(3) preprocess and (4) postprocess]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} ) .

%linkoutputpdbname% |Full path to debug info files or empty string if not applicable |
--- |--- |
| %linkoutputmapname% | full path for the map file or empty string if not applicable |
| %imgbldoutputname% | Path to the import library (for dll builds) or empty string if not applicable |
| %intermediatedir% | Intermediate build directory |
| %outputdir% | Depending on the module type `bin`  or  `lib` directory, or intermediate build directory for [Utility]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) modules. |
| %outputname% | Name of executable, dll, or library, or {{< url Module >}} name in case of  [Utility]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) modules. |
| %liboutputname% | Full path to the library or empty string if not applicable |
| %linkoutputname% | Full path to the executable or dll or empty string if not applicable |
| %securelinkoutputname% | Full path to the executable or dll or empty string if not applicable |

<a name="TargetToCommandAutoconversion"></a>
### Target to Command Auto-Conversion ###

Nant targets defined in the build steps can be automatically converted to commands by Framework.

By default such auto-conversion is turned off for backwards compatibility. It can be turned on for each target name

 by specifying an option in {{< url buildtype >}} optionset

 - `[name of the target].nantbuildonly` =&quot;false&quot;

or by specifying properties:

 - {{< url groupname >}}`.prebuildtarget.[name of the target].nantbuildonly` = &quot;false&quot;
 - {{< url groupname >}}`.prelinktarget.[name of the target].nantbuildonly` = &quot;false&quot;
 - {{< url groupname >}}`.postbuildtarget.[name of the target].nantbuildonly` = &quot;false&quot;

##### Related Topics: #####
-  [See BuildSteps example]( {{< ref "reference/examples.md" >}} ) 
