---
title: < CallTargetIfOutOfDate > Task
weight: 1173
---
## Syntax
```xml
<CallTargetIfOutOfDate InputFileset="" OutputFileset="" DummyOutputFile="" DependencyFile="" TargetName="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Compares inputs against outputs to determine whether to execute specified target.
Target is executed if any of input files is newer than any of output files, any of output files does not exist,
or list of input dependencies does not match input dependencies from previous run.

## Remarks ##
When target does not produce any output files, DummyOutputFile can be specified.

This task is similar to task &quot;ExecuteIfOutOfDate&quot; except it executes specified target instead of script.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| InputFileset | Input dependency files. | String | True |
| OutputFileset | Output dependency files. If target does not produce any output files &quot;DummyOutputFile&quot; parameter can be specified. | String | False |
| DummyOutputFile | Contains file path. File is created automatically when task is executed and added to &quot;output dependency files&quot; list. | String | False |
| DependencyFile | File to store list of input dependency files from &quot;InputFileset&quot; parameter.<br>This list is used to check whether set of input dependencies changed from previous run. Target is executed when list changes. | String | True |
| TargetName | Name of a target to execute | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<CallTargetIfOutOfDate InputFileset="" OutputFileset="" DummyOutputFile="" DependencyFile="" TargetName="" failonerror="" verbose="" if="" unless="" />
```
