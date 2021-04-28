---
title: < GetComLibraryName > Task
weight: 1185
---
## Syntax
```xml
<GetComLibraryName libpath="" Result="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| libpath |  | String | True |
| Result | The property where the result will be stored | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<GetComLibraryName libpath="" Result="" failonerror="" verbose="" if="" unless="" />
```
