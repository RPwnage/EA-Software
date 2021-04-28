---
title: < property > Task
weight: 1116
---
## Syntax
```xml
<property name="" value="" fromfile="" readonly="" deferred="" local="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Sets a property in the current project.

## Remarks ##
NAnt uses a number of predefined properties that start with nant.* or [taskname].*.  In general you should place properties into a namespace such as global.* or ProjectName.*.

If the property name is invalid a build error will occur.

The following regular expression is used to test for valid properties:  `^([\w-\.]+)$` .  In English this means only A-Z, a-z, 0-9, &#39;_&#39;, &#39;-&#39;, and &#39;.&#39; characters are allowed.  The leading character should be a letter for readability.

The task declares the  `${property.value}`  property within the task itself.  The  `${property.value}`  property is equal to the previous value of the property if already defined, otherwise it is equal to an empty string.  By using the  `${property.value}`  property, user can easily insert/append to an existing property.

**NOTE:** If you are using this property task inside a &lt;parallel.do&gt; task or &lt;parallel.foreach&gt; task, please be aware that this property task is not thread safe.  So if you are using this task inside either of the parallel blocks, you will need to make sure that they have unique names inside each block to avoid conflict.



### Example ###
Define a  `debug`  property with the value  `true` and use the
user-defined `debug`  property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="debug" value="true"/>
    <property name="trace" value="${debug}"/>
    <fail unless="${trace} == ${debug}"/>
</project>

```


### Example ###
Define a  `copyright`  property using a nested property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="copyright">Copyright (C) 2003 Gerry Shaw</property>
    <fail if="@{StrIndexOf('${copyright}', '2003')} == -1"/>
</project>

```


### Example ###
Shows how to use the  `${property.value}`  to append to a property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="test">bar;${property.value}</property>
    <property name="test" value="foo;${property.value}"/>
    <fail unless="${test} == foo;bar;"/>
</project>

```


### Example ###
Shows how to use the  `fromfile`  attribute to read from a file into a property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo file="test.txt">Hello World!</echo>
    <property name="test" fromfile="test.txt" />
    <fail unless="'${test}' == 'Hello World!'"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of the property to set. | String | True |
| value | The value of the property. If not specified, the default will be no value. | String | False |
| fromfile | The path to a file from which content is read into the property value. | String | False |
| readonly | Indicates if the property should be read-only.  Read only properties can never be changed.  Default is false. | Boolean | False |
| deferred | Indicates if the property&#39;s value will expand encapsulated properties&#39; value at definition time or at use time. Default is false. | Boolean | False |
| local | Indicates if the property is going to be defined in a local context and thus, it will be restricted to a local scope. Default is false. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<property name="" value="" fromfile="" readonly="" deferred="" local="" failonerror="" verbose="" if="" unless="" />
```
