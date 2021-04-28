---
title: < task-generatemoduleinteropassemblies > Task
weight: 1183
---
## Syntax
```xml
<task-generatemoduleinteropassemblies module="" group="" generated-assemblies-fileset="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Helper task to generate interop assemblies for a list of COM DLL&#39;s in a module.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| module | The name of the module | String | True |
| group | The name of the group that the module is part of, defaults to runtime | String | False |
| generated-assemblies-fileset |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<task-generatemoduleinteropassemblies module="" group="" generated-assemblies-fileset="" failonerror="" verbose="" if="" unless="" />
```
