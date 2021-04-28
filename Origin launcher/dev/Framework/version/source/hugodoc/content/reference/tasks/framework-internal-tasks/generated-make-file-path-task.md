---
title: < generated-make-file-path > Task
weight: 1209
---
## Syntax
```xml
<generated-make-file-path split-by-group-names="" makefile-dir-property="" makefile-name-property="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| split-by-group-names |  | Boolean | False |
| makefile-dir-property |  | String | True |
| makefile-name-property |  | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<generated-make-file-path split-by-group-names="" makefile-dir-property="" makefile-name-property="" failonerror="" verbose="" if="" unless="" />
```
