---
title: < get-module-asset-files > Task
weight: 1204
---
## Syntax
```xml
<get-module-asset-files module-groupname="" asset-filesets-property="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| module-groupname |  | String | True |
| asset-filesets-property |  | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<get-module-asset-files module-groupname="" asset-filesets-property="" failonerror="" verbose="" if="" unless="" />
```
