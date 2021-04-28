---
title: < EchoFileSet > Task
weight: 1197
---
## Syntax
```xml
<EchoFileSet Name="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Prints the all of the files in a fileset.
This task should only be used for debugging build scripts.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| Name | The name of the fileset to print | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<EchoFileSet Name="" failonerror="" verbose="" if="" unless="" />
```
