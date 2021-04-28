---
title: < target > Task
weight: 1127
---
## Syntax
```xml
<target name="" depends="" description="" if="" hidden="" override="" allowoverride="" style="" unless="" failonerror="" verbose="" />
```
## Summary ##
Create a dynamic target.

## Remarks ##
With Framework 1.x, you can declare &lt;target&gt; within a &lt;project&gt;, but not within any
task. Moreover, the target name can&#39;t be variable. But with TargetTask, you can declare a target with variable
name, or within any task that supports probing. You declare a dynamic target, and call it using &lt;call&gt;.



### Example ###
Test to show how to use &lt;target&gt; task.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="prop" value="val"/>
    <do if="@{PropertyExists('prop')}">
        <!--
        Target conditional depends on defition of prop
        If you run command: nant conditional, you'll see conditional executed after some targets.
        Reasons:
        1. When the build runs, do is executed, hence is conditional
        2. When conditional runs, it adds a target named conditional to the target list
        3. Then the build runs the foreach, hence executing its targets
        4. And finally, the build runs conditional as the given target.
        -->
        <target name="conditional">
            <echo message="In target ${target.name}"/>
        </target>
    </do>

    <optionset name="myOptSet">
        <option name="option1" value="1"/>
        <option name="option2" value="2"/>
    </optionset>
    
    <foreach item="OptionSet" in="myOptSet" property="option" verbose="true">
        <target name="looped" hidden="true">
            <echo message="In target ${target.name}: ${option.name}=${option.value}"/>
        </target>
        <!-- Without call, looped won't be executed in the loop -->
        <call target="looped"/>
        
        <property name="targetName" value="var${option.value}"/>
        <!-- Define a target with variable name -->
        <target name="${targetName}">
            <echo message="In target ${target.name}"/>
        </target>
        <call target="${targetName}"/>
    </foreach>
</project>

```

```xml

<package>
    <frameworkVersion>2</frameworkVersion>
    <buildable>false</buildable>
</package>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of the target. | String | True |
| depends | A space separated list of target names that this target depends on. | String | False |
| description | The Target description. | String | False |
| if | If true then the target will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| hidden | Prevents the target from being listed in the project help. Default is false. | Boolean | False |
| override | Override target with the same name if it already exists. Default is &#39;false&#39;.<br>Depends on the &#39;allowoverride&#39; setting in target it tries to override. | Boolean | False |
| allowoverride | Defines whether target can be overridden by other target with same name. Default is &#39;false&#39; | Boolean | False |
| style | Style can be &#39;use&#39;, &#39;build&#39;, or &#39;clean&#39;.   See page for details. | String | False |
| unless | Opposite of if.  If false then the target will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<target name="" depends="" description="" if="" hidden="" override="" allowoverride="" style="" unless="" failonerror="" verbose="" />
```
