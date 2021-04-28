---
title: < optionset > Task
weight: 1115
---
## Syntax
```xml
<optionset name="" append="" fromoptionset="" failonerror="" verbose="" if="" unless="">
  <option />
</optionset>
```
## Summary ##
Optionset is a dictionary containing [name, value] pairs called options.

## Remarks ##
An optionset denotes a group of options or properties.
Optionsets can be specified in 2 ways:  named and unnamed.

A named optionset is created by the `<optionset>` task, and so are defined
at the project level.  They are a mechanism allowing multiple tasks to reference and modify the same group of options.
This is conceptually the same as the creation of project level properties by the `<property>` task.

An unnamed optionset, also known as an optionset &quot;element&quot;, may exist for certain tasks
other than the `<optionset>` task
(e.g. &quot;env&quot; is an optionset element in the exec task).
But an unnamed optionset is accessible only to the task that owns it.  It can&#39;t
be accessed by an arbitrary task; a named optionset is needed for that.

Attributes/parameters differ between named (task) and unnamed (element) optionsets.

##### Table showing which attributes/parameters are supported #####



```xml

Attribute   optionset Task (named)	optionset Element (unnamed)
----------------------------------------------------------------------------
name            Yes                       No
append          Yes                       No
failonerror     Yes                       No
verbose         Yes                       No
if              Yes                       No
unless          Yes                       No
                                           
fromoptionset   Yes                       Yes


```
##### Syntax for named optionsets #####



```xml

<optionset
  name=""
  append=""
  fromoptionset=""
  if=""
  unless=""
  failonerror=""
  verbose=""
  >
</optionset>

```
##### Parameters for named optionsets #####


Attribute |Description |Must Provide |
--- |--- |--- |
| name | The name of the option set. | Yes | 
| append | If append is true, the options specified by this option set task are added to the options contained in the named option set.  Options that already exist are replaced. If append is false, the named option set contains the options  specified by this option set task.  Default is &quot;true&quot;. | No | 
| fromoptionset | The name of an optionset to include. | No | 
| if | Execute only if condition is true. | No | 
| unless | Execute only if condition is false. | No | 
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | No | 
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | No | 

##### Syntax for unnamed optionset element:  exec task example #####



```xml

<exec program="cmd.exe"

  <!-- env is an optionset element of exec task -->
  <env
    fromoptionset=""
    >
  /env>
</exec>

```
##### Parameters for unnamed optionset element #####


Attribute |Description |Must Provide |
--- |--- |--- |
| fromoptionset | The name of an existing optionset to initialize from.  Default is null. | No | 

##### Nested elements for both named and unnamed optionsets: #####


###### <option> ######


Specifies an option in the optionset.

Attribute |Description |Must Provide |
--- |--- |--- |
| if | If true then the option will be added; otherwise skipped. Default is &quot;true&quot;. | No | 
| unless | Opposite of if. If false then the option will be added; otherwise skipped.  Default is &quot;false&quot;. | No | 
| name | The name of the option. | Yes | 
| value | The value of the option.  You can specify the current option value using the special `${option.value}` property.  If the option has not been defined this property will expand to an empty string. | Yes | 

##### Examples of named optionsets: #####


Specify an optionset with 2 options.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name="DefaultSettings">
        <option name="foo" value="bar"/>
        <option name="nant" value="rules"/>
    </optionset>
</project>

```
Specifies an optionset based on an exsting optionset and then changes the value of the second option.  Also shows how to get the value from an optionset using a function.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name="DefaultSettings">
        <option name="foo" value="bar"/>
        <option name="nant" value="rules"/>
    </optionset>
    <optionset name="Settings" fromoptionset="DefaultSettings">
        <option name="nant" value="${option.value} the world"/>
    </optionset>
    <echo message="nant @{OptionSetGetValue('Settings', 'nant')}"/>
</project>

```

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name="settings">
        <option name="clean" value="exclude" />
    </optionset>

    <optionset name="defaults">
        <option name="build" value="include" />
        <option name="buildall" value="include" />
        <option name="clean" value="include" />
    </optionset>

    <echo message="settings (before):" />
    <foreach item="OptionSet" in="settings" property="option">
        <echo message="'${option.name} = '${option.value}'" />
    </foreach>
    <echo message="" />

    <!-- <optionset> used ${option.value} internally, which conflicts with the option property in <foreach> -->
    <foreach item="OptionSet" in="defaults" property="option">
        <do unless="@{OptionSetOptionExists('settings', '${option.name}')}">
            <echo message="   appending: '${option.name} = '${option.value}'" />
            <optionset name="settings" append="true">
                <option name="${option.name}" value="${option.value}" />
            </optionset>
        </do>
    </foreach>
    <echo message="" />

    <echo message="settings (after):" />
    <foreach item="OptionSet" in="settings" property="option">
        <echo message="'${option.name} = '${option.value}'" />
    </foreach>
</project>

```


### Example ###
Creates a named option set which can be used by other tasks.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name="SharedLibrary">
        <option name="cc.options" value="-c -nologo"/>
        <option name="cc.program=" value="cl.exe"/>
    </optionset>
    <foreach item="OptionSet" in="SharedLibrary" property="option">
        <echo message="${option.name} = ${option.value}"/>
    </foreach>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of the option set. | String | True |
| append | If append is true, the options specified by<br>this option set task are added to the options contained in the<br>named option set.  Options that already exist are replaced.<br>If append is false, the named option set contains the options<br>specified by this option set task.<br>Default is &quot;true&quot;. | Boolean | False |
| fromoptionset | The name of a file set to include. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<optionset name="" append="" fromoptionset="" failonerror="" verbose="" if="" unless="">
  <option />
</optionset>
```
