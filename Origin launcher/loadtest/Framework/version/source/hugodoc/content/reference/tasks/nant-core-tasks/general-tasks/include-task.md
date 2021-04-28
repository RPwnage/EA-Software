---
title: < include > Task
weight: 1109
---
## Syntax
```xml
<include file="" ignoremissing="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Include an build file.

## Remarks ##
This task is used to break your build file into smaller chunks.
You can load a partial build file and have it included into the main
build file.

Any global (project level) tasks in the included build file are
executed when this task is executed.  Tasks in target elements of the
included build file are only executed if that target is executed.The project element attributes in an included build
file are ignored.If this task is used within a target, the include included file
should not have any targets (or include files with targets).  Doing so
would compromise NAnt&#39;s knowledge of available targets.

### Example ###
Include the build script in the  `other.xml`  file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <include file="Component.xml"/>
    <fail message="Included script didn't execute" unless="@{FileExists('out.txt')}"/>
</project>

```
Where  `Component.xml`  contains:


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo file="out.txt" message="The component build file."/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file | Build file to include. | String | True |
| ignoremissing | Ignore if file does not exist | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<include file="" ignoremissing="" failonerror="" verbose="" if="" unless="" />
```
