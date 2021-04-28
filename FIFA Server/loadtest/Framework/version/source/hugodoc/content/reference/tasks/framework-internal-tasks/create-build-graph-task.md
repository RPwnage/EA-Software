---
title: < create-build-graph > Task
weight: 1216
---
## Syntax
```xml
<create-build-graph build-group-names="" build-configurations="" process-generation-data="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| build-group-names |  | String | True |
| build-configurations |  | String | True |
| process-generation-data |  | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<create-build-graph build-group-names="" build-configurations="" process-generation-data="" failonerror="" verbose="" if="" unless="" />
```
