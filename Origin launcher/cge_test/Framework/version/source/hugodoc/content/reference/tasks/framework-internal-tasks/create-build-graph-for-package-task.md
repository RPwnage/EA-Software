---
title: < create-build-graph-for-package > Task
weight: 1193
---
## Syntax
```xml
<create-build-graph-for-package package="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Create a build graph for a specific package instead of current package.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| package | The package we want to create the build graph for.  If not specified, will create build graph for current package. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<create-build-graph-for-package package="" failonerror="" verbose="" if="" unless="" />
```
