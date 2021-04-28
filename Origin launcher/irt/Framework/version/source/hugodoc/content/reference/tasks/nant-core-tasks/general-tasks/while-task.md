---
title: < while > Task
weight: 1137
---
## Syntax
```xml
<while condition="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Allows wrapping of a group of tasks to be repeatedly executed based on a conditional.

### Example ###
Uses a while statement to loop over a series of numbers.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='x' value='0' />
    <while condition='${x} lte 1'>
        <eval type='Function' code="@{MathAdd('${x}', '1')}" property='x' />
    </while>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| condition | The expression used to test for termination criteria. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<while condition="" failonerror="" verbose="" if="" unless="" />
```
