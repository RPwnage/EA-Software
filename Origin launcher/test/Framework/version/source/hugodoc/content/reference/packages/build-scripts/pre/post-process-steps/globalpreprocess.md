---
title: (2) Global Preprocess Step
weight: 98
---

Global preprocess step (See step (2) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram) and its usage

<a name="GlobalPreprocessStepUsage"></a>
## Usage ##

Global pre-process targets or tasks are executed after all build scripts for the whole build are loaded
and before data defined in the build scripts are extracted by Module Processor
(See step (2) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram).

Global preprocess step is executed once even in multiconfig targets. Tasks are invoked in context of the package that sets the global post
process step. If multiple packages set the same global step only one will be executed. But all Modules, Packages and their respective Project objects
can be accessed through Build Graph. To get build graph object use method:


```c#
var buildgraph = project.BuildGraph();
```
Global pre-process targets or tasks are defined by property

 - `builgraph.global.preprocess` - target or task names, new line separated.

The values of the property can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
`buildgraph.global.preprocess` targets and tasks are invoked for
all kind of targets: generation and nant build.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
global preprocess steps don&#39;t work with tasks created using &lt;createtask&gt; only ones loaded from assemblies using taskdef (ie. C# tasks).
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Global pre-process steps are invoked once (i.e. not per config).
{{% /panel %}}
