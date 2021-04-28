---
title: < init-build-graph > Task
weight: 1214
---
## Syntax
```xml
<init-build-graph build-group-names="" build-configurations="" process-generation-data="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Verify whether we can reuse build graph or need to reset it. This is only needed when we are chaining targets.


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
<init-build-graph build-group-names="" build-configurations="" process-generation-data="" failonerror="" verbose="" if="" unless="" />
```
