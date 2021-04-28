---
title: < warn > Task
weight: 1136
---
## Syntax
```xml
<warn message="" group="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Display a warning message in the current build.

## Remarks ##
Displays a warning message and location in the build file then continues with the build.



### Example ###
Display a message to a log with the current position in the build


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <warn message="This warning is on line four."/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| message | The warning message to display. | String | False |
| group | Categorizes the warning for the framework build messaging system.<br>Allows users to disable all warnings in a category.<br>(Designed to be used only by internal Framework packages for now) | WarningId | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<warn message="" group="" failonerror="" verbose="" if="" unless="" />
```
