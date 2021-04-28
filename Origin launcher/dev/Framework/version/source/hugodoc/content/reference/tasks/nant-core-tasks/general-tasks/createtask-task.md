---
title: < createtask > Task
weight: 1092
---
## Syntax
```xml
<createtask name="" overload="" failonerror="" verbose="" if="" unless="">
  <parameters fromoptionset="" />
  <code />
</createtask>
```
## Summary ##
Create a task that is made of NAnt script and can be used by &lt;task&gt;.

## Remarks ##
Use the &lt;createtask&gt; to create a new &#39;task&#39; that is made of NAnt
script.  Then use the &lt;task&gt; task to call that task with the required parameters.

The following unique properties will be created, for use by &lt;task&gt;:


 - **Task.{name}**
 - **Task.{name}.Code**
The following named optionset will be created, for use by &lt;task&gt;:
 - **Task.{name}.Parameters**


### Example ###

```xml
<createtask name="BasicTask">
   <parameters>
       <option name="DummyParam" value="Required"/>
       <option name="Indentation"/>
   </parameters>
   <code>
       <echo message="${BasicTask.Indentation}Start BasicTask."/>
       <echo message="${BasicTask.Indentation}    DummyParam value = ${BasicTask.DummyParam}"/>
       <echo message="${BasicTask.Indentation}Finished BasicTask."/>
   </code>
</createtask>
```


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
| overload | Overload existing definition. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.OptionSet" "parameters" >}}| The set of task parameters. | {{< task "NAnt.Core.OptionSet" >}} | False |
| {{< task "NAnt.Core.XmlPropertyElement" "code" >}}| The NAnt script this task is made out from | {{< task "NAnt.Core.XmlPropertyElement" >}} | False |

## Full Syntax
```xml
<createtask name="" overload="" failonerror="" verbose="" if="" unless="">
  <parameters fromoptionset="">
    <option />
  </parameters>
  <code />
</createtask>
```
