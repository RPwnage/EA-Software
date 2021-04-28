---
title: (7) PreGenerate step
weight: 101
---

Global pregenerate step (See step (7) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram) and its usage

<a name="GlobalPreprocessStepUsage"></a>
## Usage ##

Global pre-generate targets or tasks are executed before Visual Studio solution generation starts
(See step (7) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram).

Global pregenerate step is executed once even in multiconfig targets. Tasks are invoked in context of the top package
(configuration = `${config}` ). But all Modules, Packages and their respective Project objects can be accessed
through Build Graph. To get build graph object use method:


```c#
var buildgraph = project.BuildGraph();
```
Global pregenerate targets or tasks are defined by properties

 - `backend.VisualStudio.pregenerate` - target or task names, new line separated.
 - `backend.pregenerate` - target or task names, new line separated.

The values of the property can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
Global pre-generate steps are invoked once (i.e. not per config).
{{% /panel %}}
