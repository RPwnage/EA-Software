---
title: < ClangCompilationDatabase > Task
weight: 1191
---
## Syntax
```xml
<ClangCompilationDatabase outputdir="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Generates a Clang Compilation Database


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| outputdir |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<ClangCompilationDatabase outputdir="" failonerror="" verbose="" if="" unless="" />
```
