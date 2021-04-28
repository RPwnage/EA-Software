---
title: < echo > Task
weight: 1099
---
## Syntax
```xml
<echo message="" file="" append="" loglevel="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Writes a message to the build log.

## Remarks ##
A copy of the message will be sent to every defined
build log and logger on the system.  Property references in the message will be expanded.



### Example ###
Writes message to build log.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="Hello, World!"/>
</project>

```


### Example ###
Writes message with expanded macro to build log using the implicit text element.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo>Base build directory = ${nant.project.basedir}</echo>
</project>

```


### Example ###
Writes messages to a named file, instead of the build log.

Remember that the  `append`  attribute defaults to false,
so by default echoing a message to a file will create a new instance of
that file, destroying any previously existing version.


```xml

<project xmlns="schemas/ea/framework3.xsd">
 <echo message="Everyone Talks About Whether Nobody Does Anything" file="irony.out" />
 <echo message="On a clear disk, you can seek forever." file="irony.out" append="true" />
</project>

```
After this build file executes, the file  `irony.out` will
contain

&lt;![CDATA[
Everyone Talks About Whether Nobody Does Anything
On a clear disk, you can seek forever.
]]&gt;


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| message | The message to display.  For longer messages use the inner text element of the task. | String | False |
| file | The name of the file to write the message to.  If empty write message to log. | String | False |
| append | Indicates if message should be appended to file.  Default is &quot;false&quot;. | Boolean | False |
| loglevel | The log level that the message should be printed at. (ie. quiet, minimal, normal, detailed, diagnostic) default is normal. | LogLevel | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<echo message="" file="" append="" loglevel="" failonerror="" verbose="" if="" unless="" />
```
