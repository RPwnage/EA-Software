---
title: < spu > Task
weight: 1233
---
## Syntax
```xml
<spu failonerror="" verbose="" if="" unless="" />
```
## Summary ##
(Deprecated) Contains public data declarations for ps3 &#39;spu&#39; modules in Initialize.xml.
ps3 is no longer supported by Framework, this task will eventually be removed.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<spu failonerror="" verbose="" if="" unless="" />
```
