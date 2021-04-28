---
title: < Combine > Task
weight: 1184
---
## Syntax
```xml
<Combine failonerror="" verbose="" if="" unless="" />
```
## Summary ##
An internal task used by eaconfig to setup buildtypes. Not intended to be used by end users.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<Combine failonerror="" verbose="" if="" unless="" />
```
