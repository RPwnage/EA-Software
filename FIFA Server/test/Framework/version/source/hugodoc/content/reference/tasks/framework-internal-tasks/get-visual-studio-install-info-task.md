---
title: < get-visual-studio-install-info > Task
weight: 1205
---
## Syntax
```xml
<get-visual-studio-install-info out-installed-version-property="" out-appdir-property="" out-tool-version-property="" out-redist-version-property="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| out-installed-version-property |  | String | False |
| out-appdir-property |  | String | True |
| out-tool-version-property |  | String | False |
| out-redist-version-property |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<get-visual-studio-install-info out-installed-version-property="" out-appdir-property="" out-tool-version-property="" out-redist-version-property="" failonerror="" verbose="" if="" unless="" />
```
