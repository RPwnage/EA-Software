---
title: < verify-package-signature > Task
weight: 1232
---
## Syntax
```xml
<verify-package-signature packagefiles-filesetname="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
[Deprecated] Verify if package content matches signature

## Remarks ##
This task is used to verify that the package has an up to date Signature File.
This can also be done using the target &quot;verify-signature&quot;, which is the recommended way to preform this action.
The task version is mainly for internal use and should only be used if you are writing a custom target that needs to preform additional steps.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagefiles-filesetname | Name of the fileset containing all package files | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<verify-package-signature packagefiles-filesetname="" failonerror="" verbose="" if="" unless="" />
```
