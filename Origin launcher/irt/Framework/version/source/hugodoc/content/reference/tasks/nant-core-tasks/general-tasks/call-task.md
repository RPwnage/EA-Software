---
title: < call > Task
weight: 1086
---
## Syntax
```xml
<call target="" force="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Calls a NAnt target.

### Example ###
Call the target &quot;build&quot;.


```xml

<project default="run">
 <target name="build">
  <echo message="Hello, Build!"/>
 </target>

 <target name="run">
  <call target="build"/>
 </target>
</project>

```


### Example ###
This shows how a project could &#39;compile&#39; a debug and release build using a common compile target.


```xml

<project default="build">
    <target name="compile">
      <echo message="compiling with debug = ${debug}"/>
    </target>

    <target name="build">
        <property name="debug" value="false"/>
        <call target="compile"/>
        <property name="debug" value="true"/>
        <call target="compile" force="true"/> <!-- notice the force attribute -->
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| target | NAnt target to call. | String | True |
| force | Force an Execute even if the target<br>has already been executed.  Forced execution occurs on a copy of the unique<br>target, since the original can only be executed once.<br>Default value is true. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<call target="" force="" failonerror="" verbose="" if="" unless="" />
```
