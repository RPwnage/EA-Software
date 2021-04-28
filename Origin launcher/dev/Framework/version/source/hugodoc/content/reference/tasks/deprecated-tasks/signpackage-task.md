---
title: < signpackage > Task
weight: 1233
---
## Syntax
```xml
<signpackage packagefiles-filesetname="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Creates package signature file


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagefiles-filesetname | Name of the fileset containing all package files | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<signpackage packagefiles-filesetname="" failonerror="" verbose="" if="" unless="" />
```
