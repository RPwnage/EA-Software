---
title: < mkdir > Task
weight: 1111
---
## Syntax
```xml
<mkdir dir="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Creates a directory path.

## Remarks ##
A directory specified with an absolute pathname will be
created, and any parent directories in the directory path which
do not already exist will also be created.

A directory specified with a relative pathname will be created
relative to the
location of the build file.  Any directories in the relative
directory path which do not already exist will be created.



### Example ###
Create the directory &quot;build&quot;.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="build"/>
    <fail unless="@{DirectoryExists('build')}"/>
</project>

```


### Example ###
Create the directory tree &quot;one/two/three&quot;.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="one/two/three"/>
    <fail unless="@{DirectoryExists('one/two/three')}"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| dir | The directory to create. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<mkdir dir="" failonerror="" verbose="" if="" unless="" />
```
