---
title: < update-manifest-compatibility > Task
weight: 1207
---
## Syntax
```xml
<update-manifest-compatibility version="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Updates the compatibility element to match the version of the package


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| version | Allows a build script write to specify a different/modified version to use for compatibility in place of the package version | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<update-manifest-compatibility version="" failonerror="" verbose="" if="" unless="" />
```
