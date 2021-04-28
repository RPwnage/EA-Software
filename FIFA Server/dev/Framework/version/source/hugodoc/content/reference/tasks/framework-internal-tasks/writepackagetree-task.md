---
title: < WritePackageTree > Task
weight: 1222
---
## Syntax
```xml
<WritePackageTree packagename="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
This Task prints the dependency tree for a specified package. Shows all of the packages
that are added as package dependents in the current configuration.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | The name of the package whose dependency tree should be printed. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<WritePackageTree packagename="" failonerror="" verbose="" if="" unless="" />
```
