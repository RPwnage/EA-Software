---
title: (8) PostGenerate step
weight: 102
---

Global post-generate step (See step (8) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram) and its usage

<a name="GlobalPreprocessStepUsage"></a>
## Usage ##

Global post-generate targets or tasks are executed after Visual Studio solution generation is done
(See step (8) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram).

Global postgenerate step is executed once even in multiconfig targets. Tasks are invoked in context of the top package
(configuration = `${config}` ).
But all Modules, Packages and their respective Project objects can be accessed
through Build Graph. To get build graph object use method:


```c#
var buildgraph = project.BuildGraph();
```
Global postgenerate targets or tasks are defined by properties

 - `backend.VisualStudio.postgenerate` - target or task names, new line separated.
 - `package.postgenerate` - target or task names, new line separated.

The values of the property can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
Global post-generate steps are invoked once (i.e. not per config).
{{% /panel %}}
