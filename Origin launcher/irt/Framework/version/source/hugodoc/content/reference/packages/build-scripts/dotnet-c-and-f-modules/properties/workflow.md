---
title: workflow
weight: 166
---

 `workflow` property sets Visual Studio project type to &#39;Workflow&#39;.

## Usage ##

##### groupname.csproj.workflow  (groupname.fsproj.workflow) #####


Setting this property to true for a C# module will create Workflow Visual Studio project.

Assembly references added:

   *System.Core.dll*
   *System.WorkflowServices.dll*
   *System.Workflow.Activities.dll*
   *System.Workflow.ComponentModel.dll*
   *System.Workflow.Runtime.dll*

Properties used to define workflow project:

 - {{< url groupname >}} `.csproj.workflow` - for C# modules<br><br>{{< url groupname >}} `.fsproj.workflow` - for F# modules

## Example ##


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.workflow" value="true"/>
```
