---
title: (3,4) Module Pre / Post Process Steps
weight: 99
---

Module level pre and post process steps (See steps (3) and (4) on [Execution Order]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md#ProcessStepsExecutionOrder" >}} ) diagram) and its usage

<a name="GlobalPreprocessStepUsage"></a>
## Usage ##

Module level pre and post process steps are most widely used. In Framework these steps are often used in configuration
packages as well as in package build scripts.

These steps are executed for each module involved in the build and for each configuration.

Module pre-process steps are executed before data are extracted from the corresponding Projects,
and it is possible to change nant properties, filesets, or optionsets to modify module data.
Module objects are created at this stage and available, but not populated yet.

Module post-process steps are executed after Module objects are populated with data.

Module pre/post-process targets or tasks can be defined defined by properties or
by options in{{< url buildtype >}}optionset

Legacy properties. Targets specified by these properties are executed only when
running in generation targets like vs solution generation


{{% panel theme = "info" header = "Note:" %}}
Legacy targets available only in pre-process step
{{% /panel %}}
 - `[group].[module].vcproj.prebuildtarget` - target names, new line separated.<br><br> `[group].[module].csproj.prebuildtarget` <br><br> `[group].[module].fsproj.prebuildtarget`

Options in{{< url buildtype >}}optionset

 - `preprocess` - option value defines target or task names, new line separated.
 - `postprocess` - option value defines target or task names, new line separated.


```xml
.          <optionset name="config-options-common">
.            <option name="preprocess"/>
.            <option name="postprocess">
.              pc-vc-postprocess
.              all-platforms-postprocess
.            </option>
.            <option name="buildset.link.options">
.
.              . . . . . . .
.
.          </optionset>
```
Following properties can contain tasks or target, that are executed in any type of build.

 - `eaconfig.preprocess` - target or task names, new line separated.
 - `[group].[module].preprocess` - target or task names, new line separated.

The values of the property can contain multiple targets (tasks),
each target or task name on a separate line.


{{% panel theme = "info" header = "Note:" %}}
Properties and options are processed by Framework in order they are listed above.
{{% /panel %}}

##### Related Topics: #####
-  [See BuildSteps example]( {{< ref "reference/examples.md" >}} ) 
