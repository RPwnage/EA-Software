---
title: < funcdef > Task
weight: 1107
---
## Syntax
```xml
<funcdef assembly="" override="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Loads functions from a specified assembly.

## Remarks ##
NAnt can only use .NET assemblies; other types of files which
end in .dll won&#39;t work.



### Example ###
Include the functions in an assembly.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="init">
        <funcdef assembly="MyCustomFunctions.dll"/>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| assembly | File name of the assembly containing the NAnt functions. | String | True |
| override | Override function(s) with the same name.<br>Default is false. When override is &#39;false&#39; &lt;funcdef&gt; will fail on duplicate function names. | Boolean | False |
| failonerror |  | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<funcdef assembly="" override="" failonerror="" verbose="" if="" unless="" />
```
