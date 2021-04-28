---
title: < deprecate > Task
weight: 1096
---
## Syntax
```xml
<deprecate message="" group="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Display a deprecation message in the current build.

## Remarks ##
Displays a deprecation message and location in the build file then continues with the build.



### Example ###
Displays a deprecation message and location in the build file then continues with the build.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <deprecate message="This is an example deprecate task usage."/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| message | The deprecation message to display. | String | False |
| group | Categorizes the deprecation for the framework build messaging system.<br>Allows users to disable all deprecations in a category.<br>(Designed to be used only by internal Framework packages for now) | DeprecationId | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<deprecate message="" group="" failonerror="" verbose="" if="" unless="" />
```
