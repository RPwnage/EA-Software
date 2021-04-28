---
title: < eval > Task
weight: 1100
---
## Syntax
```xml
<eval code="" type="" property="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Evaluate a block of code.

## Remarks ##
The eval task will evaluate a specified block of code and optionally store the result in a property. If
no property name is specified the result ignored. This task is useful for running functions which do not
require output.

### Example ###
Evaluate a Function.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='p' />
    <eval code="@{PropertyUndefine('p')}" type="Function" />
</project>

```


### Example ###
Evaluate a Property and place the result in a property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='a' value='Hello' />
    <property name='b' value='World' />
    
    <eval property='x' code="${a}, ${b}" type="Property" />
    <echo message='${x}' />
</project>

```


### Example ###
Evaluate an Expression and place the result in a property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <eval property='x' code="1 == 0" type="Expression" />
    <echo message='${x}' />
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| code | The code to evaluate. | String | True |
| type | The type of code to evaluate. Valid values are  `Property` ,  `Function`  and  `Expression` . | EvalType | True |
| property | The name of the property to place the result into. If none is specified result is ignored. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<eval code="" type="" property="" failonerror="" verbose="" if="" unless="" />
```
