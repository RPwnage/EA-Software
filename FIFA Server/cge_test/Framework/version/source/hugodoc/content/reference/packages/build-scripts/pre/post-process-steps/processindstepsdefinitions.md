---
title: Pre / Post Process steps definitions
weight: 103
---

Sections in this topic describe how to write your own processing steps.

<a name="PrePostProcessStepsDefinitions"></a>
## How to define Pre/Post Process Steps ##

Pre or post process steps in Framework can be targets or [C# NAnt tasks]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/processindstepsdefinitions.md" >}} ) .
For each kind of steps different NAnt properties are used, but the value format for these properties is the same,
put target or task name in the property. Multiple Targets or tasks can be defined, each on a separate line.

NAnt properties can be used to specify target or task names for pre/post steps. Module level steps
( [steps (2) and (3) in the execution order graph]( {{< ref "#ProcessStepsExecutionOrder" >}} ) )
can also be set using option on buildoptionset or a property in the build script, or both.
Processing steps defined in optionsets are executed first, and then steps defined in property(ies) are executed.

###### Example of defining (6) global postprocess step ######

```xml
.          <!-- Make sure we collect all code generation into one step before generating VS Solution or building-->
.          <property name="buildgraph.global.postprocess">
.            ${property.value}
.            collectadf
.          </property>
```
###### Specifying a pre/post precess step through buildtype optionset ######

```xml
.          <optionset name="config-options-common">
.            <option name="postprocess">
.              pc-vc-postprocess
.              all-platforms-postprocess
.            </option>
.          </optionset>
```
<a name="ProcessingStepASNAntTask"></a>
## Using NAnt Task as pre/post process step. ##

To use a NAnt task as as a processing step write a task and then load it using{{< task "NAnt.Core.Tasks.TaskDefTask" >}}

<a name="ProcessingStepASNAntTask_Global"></a>
### Global Processing NAnt Tasks ###

Global processing tasks can be derived from NAnt `Task` class. You can access BuildGraph through task `Project` object.

Following example shows how to

collect all SPU job modules from `EAAudioCore` package,
convert them to a single MakeStyle module, add t

 - Access BuildGraph() object
 - Collect all SPU job modules from `EAAudioCore` package
 - Convert collected SPU modules to a single [MakeStyle]( {{< ref "reference/packages/build-scripts/make-style-modules/_index.md" >}} ) module,<br>and insert it into build graph.
 - Set dependencies between other packages and new module.
 - Mark all originally collected SPU jobs as non buildable.


```c#
{{%include filename="/reference/packages/build-scripts/pre/post-process-steps/processindstepsdefinitions/audiospujobstomake.cs" /%}}

```

{{% panel theme = "info" header = "Note:" %}}
Task must be loaded into Framework before it can be used.
Use{{< task "NAnt.Core.Tasks.TaskDefTask" >}}to both compile task and load it.
{{% /panel %}}
Example of building tasks and loading them using `<taskdef>` 


```xml
.              <taskdef assembly="${nant.project.temproot}/MyTasls.dll">
.                <sources>
.                  <includes name="..\tasks\*.cs"/>
.                </sources>
.                <references>
.                  <includes name="System.Xml.Linq.dll" asis="true"/>
.                  <includes name="Microsoft.CSharp.dll" asis="true"/>
.                </references>
.              </taskdef>
```
<a name="ProcessingStepASNAntTask_Module"></a>
### Module Level Processing NAnt Tasks ###

Nant Tasks for module level processing must be derived from [AbstractModuleProcessorTask]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/processindstepsdefinitions.md#AbstractModuleProcessorTaskCode" >}} ) class


```c#
.              namespace EA.Eaconfig.Core
.              {
.                public abstract class AbstractModuleProcessorTask : Task, IBuildModuleTask, IModuleProcessor
```
This class contains property `Module` that provides access to the Module object, and a number
of convenience methods that can be overwritten to perform various operations on the Module object and tools included in the module.
Below are few examples of using Module level postprocess task in eaconfig:


```xml
{{%include filename="/reference/packages/build-scripts/pre/post-process-steps/processindstepsdefinitions/vc-common-postprocess.cs" /%}}

```
<a name="AbstractModuleProcessorTaskCode"></a>
### AbstractModuleProcessorTask Task ###


```c#
{{%include filename="/reference/packages/build-scripts/pre/post-process-steps/processindstepsdefinitions/abstractmoduleprocessortask.cs" /%}}

```
