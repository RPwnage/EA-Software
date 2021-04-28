---
title: < task > Task
weight: 1093
---
## Syntax
```xml
<task name="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Executes a task created by &lt;createtask&gt;.

## Remarks ##
Use &lt;createtask&gt; to create a new &#39;task&#39; that is made of NAnt
script.  Then use the &lt;task&gt; task to call that task with the required parameters.

Each parameter may be either an attribute (eg. Message in example below) or a nested text element
(eg. DefList in example below) that allows multiple lines of text.  Multiple lines may be needed for certain task elements like
&lt;includedirs&gt;, &lt;usingdirs&gt;, and &lt;defines&gt; in &lt;cc&gt;.

If &lt;createtask&gt; was not used to create a task with that name and there is a regular C# task with the same name it will execute that.
This mechanism can be used to override a default task with custom behavior or convert a xml task to a C# task without breaking users build scripts.



### Example ###
Test to show how to use &lt;createtask&gt; and &lt;task&gt;.


```xml

<!-- testtask.build -->
<project xmlns="schemas/ea/framework3.xsd">
 <property name="propdefs">
 DEFINE1
 DEFINE2
 </property>
	
    <createtask name="TestTask">
        <parameters>
            <option name="Message" value="Required"/>
            <option name="DefList" value="Required"/>
        </parameters>
        <code>
            <echo message="${TestTask.Message}"/>
      <dependent name="VisualStudio"/>
   <cc >
    <defines>
    TEST_DEF
    ${TestTask.DefList}
    </defines>
    <sources>
     <includes name="test.cpp"/>
    </sources>
   </cc>
        </code>
    </createtask>

    <foreach item="OptionSet" in="Task.TestTask.Parameters" property="parameter">
        <echo message="${parameter.name} = ${parameter.value}"/>
    </foreach>

    <echo message="${Task.TestTask.Code}"/>

    <task name="TestTask" Message="@{DateTimeToday()}">
  <DefList>
  TEST_DEFINITION
  TEST_DEFINE
  ${propdefs}
  </DefList>
 </task>
   
</project>

```

```xml

<!-- manifest.xml -->
<package>
    <frameworkVersion>2</frameworkVersion>
    <buildable>false</buildable>
</package>

```

```xml

<!-- masterconfig.xml -->
<project xmlns="schemas/ea/framework3.xsd">
 <masterversions>
  <package name="VisualStudio" version="7.1.1-5"/>
  <package name="Framework" version="dev"/>
 </masterversions>
 <config package="Framework" default="win-release"/>
</project>

```

```xml

//test.cpp
#ifndef TEST_DEFINE
#error TEST_DEFINE not defined
#endif

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of the task being declared. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<task name="" failonerror="" verbose="" if="" unless="" />
```
