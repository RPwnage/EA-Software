---
title: < choose > Task
weight: 1086
---
## Syntax
```xml
<choose failonerror="" verbose="" if="" unless="">
  <do />
</choose>
```
## Summary ##
The `choose`  task is used in conjunction with the  `do` task to express multiple conditional
statements.

## Remarks ##
The code of the first, and only the first, `do` task whose expression evaluates to true is executed.



### Example ###
If / Else conditional.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='a' value='true' />

    <choose>
        <do if='${a}'>
            <echo message='a is true' />
        </do>
        <do>
            <echo message='a is false' />
        </do>
    </choose>
</project>

```


### Example ###
If / ElseIf / Else conditional.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='a' value='false' />
    <property name='b' value='true' />

    <choose>
        <do if='${a}'>
            <echo message='a is true' />
        </do>
        <do if='${b}'>
            <echo message='a is false and b is true' />
        </do>
        <do>
            <echo message='a is false and b is false' />
        </do>
    </choose>
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
<choose failonerror="" verbose="" if="" unless="">
  <do />
</choose>
```
