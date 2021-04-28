---
title: < generate-makefile > Task
weight: 1217
---
## Syntax
```xml
<generate-makefile startup-config="" configurations="" generate-single-config="" split-by-group-names="" use-compiler-wrapper="" for-config-systems="" warn-about-csharp-modules="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| startup-config |  | String | True |
| configurations |  | String | True |
| generate-single-config |  | Boolean | False |
| split-by-group-names |  | Boolean | False |
| use-compiler-wrapper |  | Boolean | False |
| for-config-systems |  | String | False |
| warn-about-csharp-modules |  | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<generate-makefile startup-config="" configurations="" generate-single-config="" split-by-group-names="" use-compiler-wrapper="" for-config-systems="" warn-about-csharp-modules="" failonerror="" verbose="" if="" unless="" />
```
