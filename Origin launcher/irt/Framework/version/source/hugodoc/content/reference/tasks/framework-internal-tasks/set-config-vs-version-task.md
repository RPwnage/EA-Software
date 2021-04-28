---
title: < set-config-vs-version > Task
weight: 1206
---
## Syntax
```xml
<set-config-vs-version failonerror="" verbose="" if="" unless="" />
```
## Summary ##
This task sets some properties that can be used if you need to execute your build
differently depending on the version of Visual Studio that is being used.

## Remarks ##
A property called config-vs-version is set to &quot;14.0.0&quot; if some variant of VisualStudio
2015 is being used, &quot;15.0.0&quot; if some variant of VisualStudio 2017 is being used, or
&quot;16.0.0&quot; if some variant of VisualStudio 2019 is being used.

In addition to the config-vs-version property, some legacy properties called
package.eaconfig.isusingvc[8, 9, 10, 11] are also set to true.  They are cumulative,
meaning that if package.eaconfig.isusingvc11 is set, it means that the ones for versions
8, 9, and 10 are also set to true.  We are




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<set-config-vs-version failonerror="" verbose="" if="" unless="" />
```
