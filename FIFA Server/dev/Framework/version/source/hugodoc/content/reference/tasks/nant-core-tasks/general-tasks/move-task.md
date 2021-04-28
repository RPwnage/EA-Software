---
title: < move > Task
weight: 1110
---
## Syntax
```xml
<move file="" tofile="" todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" symlinkifpossible="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</move>
```
## Summary ##
Moves a file or file set to a new location.

## Remarks ##
Files are only moved if the source file is newer than the destination file, or if the destination file does not exist.  This applies to files matched by a file set as well as files specified individually.

You can explicitly overwrite files with the overwrite attribute.File sets are used to select groups of files to move. To use a file set, the todir attribute must be set.



### Example ###
Move a single file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo file="myfile.txt" message="A file to move."/>
    <move file="myfile.txt" tofile="mytarget.txt"/>
</project>

```


### Example ###
Move a set of files.


```xml

<project default="MoveFiles">
   <target name="MoveFiles">
      <mkdir dir="C:\test"/>
      <move todir="C:\test">
          <fileset basedir=".">
              <includes name="*.txt"/>
          </fileset>
      </move>
   </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file | The file to transfer in a single file transfer operation. | String | False |
| tofile | The file to transfer to in a single file transfer operation. | String | False |
| todir | The directory to transfer to, when transferring a file set. | String | False |
| overwrite | Overwrite existing files even if the destination files are newer. Defaults is false. | Boolean | False |
| clobber | Allow hidden and read-only files to be overwritten if appropriate (i.e. if source is newer or overwrite is set to true). Default is false. | Boolean | False |
| maintainattributes | Maintain file attributes of overwritten files. By default and if destination file does not exist file attributes are carried over from source file. Default is false. | Boolean | False |
| flatten | Flatten directory structure when transferring a file set. All files are placed in the todir, without duplicating the directory structure. | Boolean | False |
| retrycount | Number of times to retry the copy operation if copy fails. | Int32 | False |
| retrydelay | Length of time in milliseconds between retry attempts. | Int32 | False |
| hardlinkifpossible | Tries to create a hard link to the source file rather than copying the entire contents of the file.<br>If unable to create a hard link it simply copies the file. | Boolean | False |
| symlinkifpossible | Tries to create a hard link to the source file rather than copying the entire contents of the file.<br>If unable to create a hard link it simply copies the file. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| The (single) file set to be copied.<br>Note the todir attribute must be set, and that a fileset<br>may include another fileset. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<move file="" tofile="" todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" symlinkifpossible="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</move>
```
