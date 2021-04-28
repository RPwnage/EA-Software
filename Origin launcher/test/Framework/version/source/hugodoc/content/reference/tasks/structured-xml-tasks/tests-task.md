---
title: < tests > Task
weight: 1038
---
## Syntax
```xml
<tests />
```
## Summary ##
Contains &#39;test&#39; modules in structured XML. Module definitions inside this tag will belong to &#39;test&#39; group.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<tests failonerror="" verbose="" if="" unless="" />
```
