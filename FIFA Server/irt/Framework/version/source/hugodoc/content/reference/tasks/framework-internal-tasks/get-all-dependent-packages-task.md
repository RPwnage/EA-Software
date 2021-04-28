---
title: < get-all-dependent-packages > Task
weight: 1203
---
## Syntax
```xml
<get-all-dependent-packages property="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Extract a list of dependent packages from build graph for all top modules in current package.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| property | Put the result this this property | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<get-all-dependent-packages property="" failonerror="" verbose="" if="" unless="" />
```
