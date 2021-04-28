---
title: < do > Task
weight: 1100
---
## Syntax
```xml
<do failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Allows wrapping of a group of tasks to be executed based on a conditional.

## Remarks ##
Use the do task in conjunction with the if and unless attributes to execute a series
of tasks based on the condition.

Another use for the task is when you want to execute a task only if a property is
defined.  Normally NAnt will try to expand the property at the same time performing
the if/unless condition check.



### Example ###
Simple example of wrapping a series of tasks with a single condition.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <do if="true">
        <echo message="Hello, World!"/>
        <echo message="Goodbye, Cruel World!"/>
    </do>
</project>

```


### Example ###
Shows how to execute a task that takes a property but only if that property is defined.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="foo" value="bar"/>
    <do if="@{PropertyExists('foo')}">
        <echo message="${foo}"/>
    </do>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<do failonerror="" verbose="" if="" unless="" />
```
