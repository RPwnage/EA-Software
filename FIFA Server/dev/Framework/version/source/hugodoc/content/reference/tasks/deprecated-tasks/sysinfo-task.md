---
title: < sysinfo > Task
weight: 1225
---
## Syntax
```xml
<sysinfo prefix="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
(Deprecated) Set properties with system information.

## Remarks ##
This task is Deprecated and no longer preforms any function. All of the properties that were setup by this task are now setup automatically for each project.

Sets a number of properties with information about the system environment.  The intent of this task is for nightly build logs to have a record of the system information that the build was performed on.

Property |Value |
--- |--- |
| sys.clr.version | Common Language Runtime version number. | 
| sys.env.* | Environment variables, stored both in upper case or their original case.(e.g., sys.env.Path or sys.env.PATH). | 
| sys.os.folder.system | The System directory. | 
| sys.os.folder.temp | The temporary directory. | 
| sys.os.platform | Operating system platform ID. | 
| sys.os.version | Operating system version. | 
| sys.os | Operating system version string. | 



### Example ###
Register the properties with the default property prefix.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <sysinfo/>
    <echo message="Your operating system is: ${sys.os}"/>
</project>

```


### Example ###
Register the properties without a prefix.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <sysinfo prefix=""/>
    <echo message="Your operating system is: ${os}"/>
</project>

```


### Example ###
Register properties and display the values set.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <sysinfo verbose="true"/>
</project>

```


### Example ###
Register properties and display the values set.


```xml

<project default="build">
 <target name="build">
  <echo message="sys.env.windir == ${sys.env.windir}"/>
 </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| prefix | The string to prefix the property names with.  Default is &quot;sys.&quot; | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<sysinfo prefix="" failonerror="" verbose="" if="" unless="" />
```
