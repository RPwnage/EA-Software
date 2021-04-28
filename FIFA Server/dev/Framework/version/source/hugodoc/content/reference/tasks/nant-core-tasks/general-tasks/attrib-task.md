---
title: < attrib > Task
weight: 1082
---
## Syntax
```xml
<attrib file="" archive="" hidden="" normal="" readonly="" system="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</attrib>
```
## Summary ##
Changes the file attributes of a file or set of files.

## Remarks ##
The  `attrib` task does not conserve prior file attributes.  Any specified file
attributes are set; all other attributes are switched off.



### Example ###
Set the  `ReadOnly`  attribute to true for the specified file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt"/>
    <attrib file="foo.txt" readonly="true"/>
    <fail
        message="'foo.txt' not set to readonly."
        unless="@{FileCheckAttributes('foo.txt', 'ReadOnly')}"
        />
</project>

```


### Example ###
Clean any flags on the specified file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt"/>

    <!-- first let's add some attributes -->
    <attrib file="foo.txt" readonly="true" hidden="true"/>

    <!-- now let's clear them -->
    <attrib file="foo.txt" normal="true"/>
    <fail
        message="'foo.txt' not set to readonly."
        if="@{FileCheckAttributes('foo.txt', 'ReadOnly Hidden')}"
        />
</project>

```


### Example ###
Set the normal file attributes to all executable files in the current
directory and sub-directories.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <attrib normal="true">
        <fileset>
            <includes name="**/*.exe"/>
            <includes name="**/*.dll"/>
        </fileset>
    </attrib>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file | The name of the file which will have its attributes set.  This is provided as an alternate to using the task&#39;s fileset. | String | False |
| archive | Set the archive attribute.  Default is &quot;false&quot;. | Boolean | False |
| hidden | Set the hidden attribute.  Default is &quot;false&quot;. | Boolean | False |
| normal | Set the normal file attributes.  This attribute is valid only if used alone.  Default is &quot;false&quot;. | Boolean | False |
| readonly | Set the read only attribute.  Default is &quot;false&quot;. | Boolean | False |
| system | Set the system attribute.  Default is &quot;false&quot;. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| All the files in this fileset will have their file attributes set. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<attrib file="" archive="" hidden="" normal="" readonly="" system="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</attrib>
```
