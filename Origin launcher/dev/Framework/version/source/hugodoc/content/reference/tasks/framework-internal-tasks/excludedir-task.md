---
title: < ExcludeDir > Task
weight: 1199
---
## Syntax
```xml
<ExcludeDir Fileset="" Directory="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Exclude a directory from an existing fileset


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| Fileset | The name of the fileset that we want to exclude from | String | True |
| Directory | The name of the directory that we want to exclude | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<ExcludeDir Fileset="" Directory="" failonerror="" verbose="" if="" unless="" />
```
