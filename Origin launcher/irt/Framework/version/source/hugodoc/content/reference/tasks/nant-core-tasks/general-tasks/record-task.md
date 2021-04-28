---
title: < record > Task
weight: 1119
---
## Syntax
```xml
<record property="" silent="" file="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
A task that records the build&#39;s output to a property or file.

## Remarks ##
This task allows you to record the build&#39;s output, or parts of it to
a named property or file.

### Example ###
Simple example recording build log to a named property.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <record property="log">
        <echo message="This gets saved in the log property"/>
    </record>
    <fail
        unless="@{StrIndexOf('${log}', 'This gets saved in the log property')} != 0"
        message="Log wasn't saved: ${log}"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| property | Name of the property to record output. | String | False |
| silent | If set to true, no other output except of property is produced. Console or other logs are suppressed | Boolean | False |
| file | The name of a file to write the output to | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<record property="" silent="" file="" failonerror="" verbose="" if="" unless="" />
```
