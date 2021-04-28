---
title: < printbuildstats > Task
weight: 1088
---
## Syntax
```xml
<printbuildstats BuildPackages="" BuildVersion="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Prints colected build statistics


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| BuildPackages | Package list that was in this build. | String | False |
| BuildVersion | A version string for this build. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<printbuildstats BuildPackages="" BuildVersion="" failonerror="" verbose="" if="" unless="" />
```
