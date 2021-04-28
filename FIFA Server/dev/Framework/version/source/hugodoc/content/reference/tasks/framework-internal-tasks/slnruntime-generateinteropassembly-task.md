---
title: < slnruntime-generateinteropassembly > Task
weight: 1181
---
## Syntax
```xml
<slnruntime-generateinteropassembly comdll="" interopdll="" namespace="" keyfile="" message="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Generates a single interop assembly from a COM DLL. Dependency-checking is performed.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| comdll |  | String | True |
| interopdll |  | String | True |
| namespace |  | String | True |
| keyfile |  | String | False |
| message |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<slnruntime-generateinteropassembly comdll="" interopdll="" namespace="" keyfile="" message="" failonerror="" verbose="" if="" unless="" />
```
