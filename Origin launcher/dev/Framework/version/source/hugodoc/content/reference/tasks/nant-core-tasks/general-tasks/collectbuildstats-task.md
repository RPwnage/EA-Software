---
title: < collectbuildstats > Task
weight: 1087
---
## Syntax
```xml
<collectbuildstats label="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Collects statistics for a group of tasks wrapped by this element


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| label | Unique label name for this set of build stats. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<collectbuildstats label="" failonerror="" verbose="" if="" unless="" />
```
