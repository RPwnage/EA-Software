---
title: < task-generatemodulewebreferences > Task
weight: 1186
---
## Syntax
```xml
<task-generatemodulewebreferences module="" group="" output="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Use wsdl to generate proxy classes for ASP.NET web services. The generated code is placed in ${workingdir}.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| module | The name of the module | String | True |
| group | The group the module is in | String | False |
| output | The fileset (C#) or property (managed) where the output will be stored | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<task-generatemodulewebreferences module="" group="" output="" failonerror="" verbose="" if="" unless="" />
```
