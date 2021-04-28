---
title: < generate.master-packages-optionset > Task
weight: 1177
---
## Syntax
```xml
<generate.master-packages-optionset optionsetname="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
creates optionset that contains all packages listed in masterconfig with key=&#39;package name&#39;, value=&#39;version&#39;.

## Remarks ##
Package version exceptions are evaluated against task Project instance.

Has static C# interface: public static OptionSet Execute(Project project).




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| optionsetname | Name of the optionset to generate | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<generate.master-packages-optionset optionsetname="" failonerror="" verbose="" if="" unless="" />
```
