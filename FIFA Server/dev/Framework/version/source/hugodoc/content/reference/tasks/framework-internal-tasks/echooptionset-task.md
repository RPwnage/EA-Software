---
title: < EchoOptionSet > Task
weight: 1198
---
## Syntax
```xml
<EchoOptionSet Name="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Prints the all of the options in an optionset.
This task should only be used for debugging build scripts.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| Name | The name of the optionset to print | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<EchoOptionSet Name="" failonerror="" verbose="" if="" unless="" />
```
