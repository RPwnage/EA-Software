---
title: < delete > Task
weight: 1096
---
## Syntax
```xml
<delete file="" dir="" failonmissing="" quiet="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</delete>
```
## Summary ##
Deletes a file, file set or directory.

## Remarks ##
Deletes either a single file, all files in a specified directory and its sub-directories, or a set of files specified by one or more file sets.

If the file attribute is set then the file set contents will be ignored.  To delete the files in the file set, omit the file attribute in the delete element.All items specified including read-only files and directories are
deleted.  If an item cannot be deleted because of some other reason
the task will fail if &quot;failonmissing&quot; is true.

### Example ###
Delete a single file.  If the file does not exist the task does nothing.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt"/>
    <delete file="foo.txt"/>
    <fail if="@{FileExists('foo.txt')}"/>
</project>

```


### Example ###
Delete a directory and the contents within.  If the directory does not
exist the task will fail.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="c:\bar69"/>
    <touch file="c:\bar69\one.txt"/>
    <touch file="c:\bar69\two.txt"/>
    <delete dir="c:\bar69" failonmissing="true"/>
    <fail if="@{DirectoryExists('c:\bar69')}"/>
</project>

```


### Example ###
Delete a set of files.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="apple.txt"/>
    <touch file="banana.txt"/>
    <touch file="carot.bin"/>
    <delete>
        <fileset>
            <includes name="*.txt"/>
        </fileset>
    </delete>
    <fail if="@{FileExists('apple.txt')}"/>
    <fail unless="@{FileExists('carot.bin')}"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file | The file to delete. Applies only to the single file delete operation. | String | False |
| dir | The directory to delete. | String | False |
| failonmissing | If true the task will fail if a file or directory specified is not present.  Default is false. | Boolean | False |
| quiet | If true then be quiet - don&#39;t report on the deletes. Default is false | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| All the files in the file set will be deleted. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<delete file="" dir="" failonmissing="" quiet="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</delete>
```
