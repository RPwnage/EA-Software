---
title: < fail > Task
weight: 1102
---
## Syntax
```xml
<fail type="" message="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Exit the current build.

## Remarks ##
Exits the current build optionally printing additional information.



### Example ###
Will exit the current build displaying a message if the property `should-fail`  is set to true.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="should-fail" value="false"/>
    <fail if="${should-fail}" message="Because 'should-fail' was true."/>

    <trycatch>
      <try>
       <fail unless="@{FileExists(${filename})}" type="FileNotFoundException" message="File ${filename} was not found."/>
       <!-- do something with the file in ${filename}-->
      </try>
      <catch exception="FileNotFoundException">
        <echo message="File ${filename} could not be properly treated because it was not found"/>
      </catch>
    </trycatch>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| type | Type of the exception thrown, for use with the &lt;trycatch&gt; task. | String | False |
| message | A message giving further information on why the build exited. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<fail type="" message="" failonerror="" verbose="" if="" unless="" />
```
