---
title: < ExecuteIfOutOfDate > Task
weight: 1171
---
## Syntax
```xml
<ExecuteIfOutOfDate DummyOutputFile="" DependencyFile="" failonerror="" verbose="" if="" unless="">
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <code />
</ExecuteIfOutOfDate>
```
## Summary ##
Compares inputs against outputs to determine whether to execute specified target.
Target is executed if any of input files is newer than any of output files, any of output files does not exist,
or list of input dependencies does not match input dependencies from previous run.

## Remarks ##
This task is similar to task &quot;CallTargetIfOutOfDate&quot; except it executes arbitrary script from &quot;code&quot; instead of target.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| DummyOutputFile | If specified, this file is added to the list of output dependencies. Timestamp of DummyOutputFile file is updated when this task code is executed. | String | False |
| DependencyFile | The list of &quot;inputs&quot; is stored in the file with this name. It is used to check for missing/added input files during consecutive runs. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "inputs" >}}| Set of input files to check against. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "outputs" >}}| Set of output files to check against. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "code" >}}| The NAnt script to execute. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<ExecuteIfOutOfDate DummyOutputFile="" DependencyFile="" failonerror="" verbose="" if="" unless="">
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </inputs>
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </outputs>
  <code />
</ExecuteIfOutOfDate>
```
