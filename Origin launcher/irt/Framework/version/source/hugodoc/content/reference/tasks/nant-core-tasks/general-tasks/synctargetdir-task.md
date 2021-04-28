---
title: < synctargetdir > Task
weight: 1126
---
## Syntax
```xml
<synctargetdir todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" deleteemptydirs="" quiet="" failonerror="" verbose="" if="" unless="">
  <copyfilesonly />
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <fileset-names />
  <excludefiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</synctargetdir>
```
## Summary ##
Synchronizes target directory file set(s) to a target dir and removes files from the target dir that aren&#39;t present in the file set(s).

## Remarks ##
By default, files are copied if the source file is newer than the destination file, or if the
destination file does not exist. However, you can explicitly overwrite (newer) files with the `overwrite`  attribute set to true.

A file set(s), defining groups of files using patterns, is copied.
All the files matched by the file set will be
copied to that directory, preserving their associated directory structure.

Any directories are created as needed by the  `synctargetdir`  task.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| todir | The directory to transfer to, when transferring a file set. | String | True |
| overwrite | Overwrite existing files even if the destination files are newer. Defaults is false. | Boolean | False |
| clobber | Allow hidden and read-only files to be overwritten if appropriate (i.e. if source is newer or overwrite is set to true). Default is false. | Boolean | False |
| maintainattributes | Maintain file attributes of overwritten files. By default and if destination file does not exist file attributes are carried over from source file. Default is false. | Boolean | False |
| flatten | Flatten directory structure when transferring a file set. All files are placed in the todir, without duplicating the directory structure. | Boolean | False |
| retrycount | Number of times to retry the copy operation if copy fails. | Int32 | False |
| retrydelay | Length of time in milliseconds between retry attempts. | Int32 | False |
| hardlinkifpossible | Tries to create a hard link to the source file rather than copying the entire contents of the file.<br>If unable to create a hard link it simply copies the file. | Boolean | False |
| deleteemptydirs | Delete empty folders under the target directory. Default:true | Boolean | False |
| quiet | Do not print any info. Default:false | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.PropertyElement" "copyfilesonly" >}}| Just copy the files only but do not do directory/fileset synchronization. Default:false | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| The (single) file set to be copied.<br>Note the todir attribute must be set, and that a fileset<br>may include another fileset. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "fileset-names" >}}| Names of the filesets to synchronize | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.FileSet" "excludefiles" >}}| Files to exclude from synchronization. Files in this fileset will<br>not be deleted even if these files aren&#39;t present in the input files to copy.<br>Note: &#39;excludefiles&#39; fileset does not affect copy phase. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<synctargetdir todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" deleteemptydirs="" quiet="" failonerror="" verbose="" if="" unless="">
  <copyfilesonly />
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
  <fileset-names />
  <excludefiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </excludefiles>
</synctargetdir>
```
