---
title: < call-base > Task
weight: 1083
---
## Syntax
```xml
<call-base failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Calls a NAnt target.

### Example ###
Call the target &quot;build&quot;.


```xml

<project default="run">
 <target name="MyTarget" allowoverride="true">
  <echo message="Hello from base target"/>
 </target>

 <target name="MyTarget" override="true">
  <call-base />
 </target>
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
<call-base failonerror="" verbose="" if="" unless="" />
```
