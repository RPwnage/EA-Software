---
title: < MergeOptionset > Task
weight: 1186
---
## Syntax
```xml
<MergeOptionset OriginalOptionset="" FromOptionset="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
NAnt custom task which merges options from a NAnt code-build script.
This differs from the built-in Framework mergeoption in that it is using C#
instead of nAnt tasks to do the work in order to increase performance


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| OriginalOptionset |  | String | True |
| FromOptionset |  | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<MergeOptionset OriginalOptionset="" FromOptionset="" failonerror="" verbose="" if="" unless="" />
```
