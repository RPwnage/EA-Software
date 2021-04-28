---
title: < tools > Task
weight: 1040
---
## Syntax
```xml
<tools />
```
## Summary ##
Contains &#39;tool&#39; modules in structured XML. Module definitions inside this tag will belong to &#39;tool&#39; group.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<tools failonerror="" verbose="" if="" unless="" />
```
