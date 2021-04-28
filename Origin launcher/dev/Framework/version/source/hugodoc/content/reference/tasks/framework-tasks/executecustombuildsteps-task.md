---
title: < ExecuteCustomBuildSteps > Task
weight: 1173
---
## Syntax
```xml
<ExecuteCustomBuildSteps groupname="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Executes a named custom build step.

## Remarks ##
Note: This task is intended for internal use within eaconfig to allow
certain build steps to be called at the beginning or end of specific
eaconfig targets.

Executes a custom build step which may either consist of NAnt tasks or
batch/shell script commands.

The custom build step is defined using the property
runtime.[module].vcproj.custom-build-tool.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| groupname | The name of the module whose build steps we want to execute. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<ExecuteCustomBuildSteps groupname="" failonerror="" verbose="" if="" unless="" />
```
