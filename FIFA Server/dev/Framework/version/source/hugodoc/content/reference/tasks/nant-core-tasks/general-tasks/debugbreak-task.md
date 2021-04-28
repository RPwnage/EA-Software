---
title: < debugbreak > Task
weight: 1093
---
## Syntax
```xml
<debugbreak failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Triggers a debug break so that you can stop execution at a sepecific point in a build script and inspect the state in the debugger.

## Remarks ##
Useful when trying to debug a build script in order to stop execution a specific point to inspect the state of framework properties.
For more information about debugging framework see the &quot;Debugging Framework&quot; guide in the User Guides section of the Framework documentation.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<debugbreak failonerror="" verbose="" if="" unless="" />
```
