---
title: (6) Global Postprocess Step
weight: 100
---

Global postprocess step (See step (6) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram) and its usage

<a name="GlobalPreprocessStepUsage"></a>
## Usage ##

Global post-process targets or tasks are executed after build graph is computed and before build or generation tasks are started
(See step (6) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram).

Global postprocess step is executed once even in multiconfig targets. Tasks are invoked in context of the package that sets the global post
process step. If multiple packages set the same global step only one will be executed. But all Modules, Packages and their respective Project objects
can be accessed through Build Graph. To get build graph object use method:


```c#
var buildgraph = project.BuildGraph();
```
Global postprocess step usually is implemented as task because all input from XML scripts is now converted to C# objects in the build graph.
This step can be used to alter build graph: add/remove dependencies between module, add or remove modules, etc.

Global post-process targets or tasks are defined by property

##### buildgraph.global.postprocess #####
  

target or task names, new line separated.

The values of the property can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
`buildgraph.global.postprocess` targets and tasks are invoked for
all kind of targets: generation and nant build.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
global preprocess steps don&#39;t work with tasks created using &lt;createtask&gt; only ones loaded from assemblies using taskdef (ie. C# tasks).
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Global pre-process steps are invoked once (i.e. not per config).
{{% /panel %}}
