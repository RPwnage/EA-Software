---
title: < async.wait > Task
weight: 1081
---
## Syntax
```xml
<async.wait key="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| key | Unique key string. When using global context make sure that key string does<br>not collide with keys that may be defined in other packages.<br>Using &quot;package.[package name].&quot; prefix is a good way to ensure unique values. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<async.wait key="" failonerror="" verbose="" if="" unless="" />
```
