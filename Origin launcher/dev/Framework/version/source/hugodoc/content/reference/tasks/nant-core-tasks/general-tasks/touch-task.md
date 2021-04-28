---
title: < touch > Task
weight: 1127
---
## Syntax
```xml
<touch file="" millis="" datetime="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</touch>
```
## Summary ##
Touch a file and/or fileset(s); corresponds to the Unixtouchcommand.

## Remarks ##
If the file exists,  `touch`  changes the last access and last write timestamps to the current time.  If no file of that name exists,  `touch`  will create an empty file with that name, and set the create, last access and last writetimestamps to the current time.

### Example ###
Touch a file using the current time.  If the file does not exist it will be created..


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt"/>
</project>

```


### Example ###
Touch all executable files in the current directory and its subdirectories.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch>
        <fileset>
            <includes name="**/*.exe"/>
            <includes name="**/*.dll"/>
        </fileset>
    </touch>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file | Assembly Filename (required unless a fileset is specified). | String | False |
| millis | Specifies the new modification time of the file in milliseconds since midnight Jan 1 1970.<br>The FAT32 file system has limitations on the date value it can hold. The smallest is<br>```xml<br>"12/30/1979 11:59:59 PM"<br>```<br>, and largest is <br>```xml<br>"12/30/2107, 11:59:58 PM"<br>```<br>. | String | False |
| datetime | Specifies the new modification time of the file in the format MM/DD/YYYY HH:MM AM_or_PM. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| Fileset to use instead of single file. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<touch file="" millis="" datetime="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</touch>
```
