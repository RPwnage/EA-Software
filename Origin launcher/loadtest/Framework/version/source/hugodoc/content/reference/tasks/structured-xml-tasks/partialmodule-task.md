---
title: < PartialModule > Task
weight: 1025
---
## Syntax
```xml
<PartialModule name="" comment="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Declares partial module definition. NOTE: schema and intellisense are generated for &#39;native&#39; PartialModule only, for DotNet partial modules use &#39;CsharpLibrary&#39; as a template.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name |  | String | True |
| comment |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<PartialModule name="" comment="" failonerror="" verbose="" if="" unless="" />
```
