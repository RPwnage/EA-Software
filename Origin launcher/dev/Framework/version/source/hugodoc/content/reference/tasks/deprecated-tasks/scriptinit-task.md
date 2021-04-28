---
title: < ScriptInit > Task
weight: 1230
---
## Syntax
```xml
<ScriptInit PackageName="" IncludeDirs="" Libs="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
(Deprecated) Sets up a number of default public data properties. This is deprecated, use the &#39;publicdata&#39; task instead.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| PackageName | The name of the package | String | True |
| IncludeDirs |  | String | False |
| Libs |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<ScriptInit PackageName="" IncludeDirs="" Libs="" failonerror="" verbose="" if="" unless="" />
```
