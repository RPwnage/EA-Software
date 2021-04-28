---
title: (1) Package Level Preprocess
weight: 97
---

Package level pre and post process steps (See step (1) on  [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} )  diagram) and their usage

<a name="PackageLevelProcessUsage"></a>
## Usage ##

Package level pre-process targets or tasks are executed right before loaded package build script
is initially processed by `load-package`  task to extract modules {{< url buildtype >}}and dependencies.

The reason this step is executed so early (See step (1) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram),
unlike all other pre and post process steps that are executed after all build scripts are loaded,
is backwards compatibility with some existing game scripts. These scripts set modules{{< url buildtype >}}in the package level preprocess targets instead of setting it in the build script itself

Package level pre-process targets or tasks can be defined by one of these properties

 - `package.preprocess` - target or task names, new line separated.

Legacy properties. Targets specified by these properties are executed only when
running in generation targets like vs solution generation

 - `vcproj.prebuildtarget` - targetnames, new line separated.
 - `csproj.prebuildtarget` - targetnames, new line separated.

{{% panel theme = "info" header = "Note:" %}}
Targets specified by Legacy properties properties are executed only in generation
targets (Visual Studio Solution generation, XCode project generation, etc.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Legacy properties can contain only target names.
{{% /panel %}}
The values of the properties above can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
`package.preprocess` targets and tasks are invoked for all kind of targets: generation and nant build.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Package preprocess steps are invoked for each configuration independently
{{% /panel %}}
### Package Level Post Process Steps ###

Package level post process steps (See step (5) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram)
are not implemented in the current version of the Framework.

