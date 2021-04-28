---
title: < collect-masterconfig-packages > Task
weight: 1189
---
## Syntax
```xml
<collect-masterconfig-packages groupname="" target-property="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Gets a list of packages in a specific group in the masterconfig file and stores it in a property


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| groupname | The name of a group in the masterconfig config file | String | True |
| target-property | The name of a property where the list will be stored | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<collect-masterconfig-packages groupname="" target-property="" failonerror="" verbose="" if="" unless="" />
```
