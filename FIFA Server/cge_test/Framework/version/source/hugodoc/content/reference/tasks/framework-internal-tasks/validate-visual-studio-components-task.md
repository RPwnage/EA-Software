---
title: < validate-visual-studio-components > Task
weight: 1209
---
## Syntax
```xml
<validate-visual-studio-components components="" failonmissing="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Ensures that the user&#39;s Visual Studio installation has the specified components.
Determines the Visual Studio installation to check by using the vsversion and package.VisualStudio.AllowPreview properties


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| components | The components to ensure are part of the user&#39;s Visual Studio installation | String | True |
| failonmissing | If false, will not fail if Visual Studio is not installed. Default is true. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<validate-visual-studio-components components="" failonmissing="" failonerror="" verbose="" if="" unless="" />
```
