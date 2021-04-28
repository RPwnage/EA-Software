---
title: < copy > Task
weight: 1089
---
## Syntax
```xml
<copy file="" tofile="" todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" symlinkifpossible="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</copy>
```
## Summary ##
Copies a file or file set to a new location.

## Remarks ##
By default, files are copied if the source file is newer than the destination file, or if the
destination file does not exist. However, you can explicitly overwrite (newer) files with the `overwrite`  attribute set to true.

A file set, defining groups of files using patterns, can be copied if
the `todir` attribute is set.  All the files matched by the file set will be
copied to that directory, preserving their associated directory structure.
Beware that `copy` handles only a single fileset, but note that
a fileset may include another fileset.

Any directories are created as needed by the  `copy`  task.

There is another task available that is similar to the copy task, the &lt;synctargetdir&gt; task.
The difference is that it tries to ensure two directories are in sync by only copying
files with changes and deleting files that are no longer existent in the source fileset.



### Example ###
Copy a single file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo file="source.txt">A temp file.</echo>
    <copy file="source.txt" tofile="destination.txt"/>
    <fail message="File did not copy." unless="@{FileExists('destination.txt')}"/>
</project>

```


### Example ###
Copy a set of files to a new directory.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- create some test files -->
    <touch file="a.txt"/>
    <touch file="b.txt"/>
    <touch file="c.txt"/>
    
    <!-- copy the files to new directory (directory will get created) -->
    <copy todir="sub">
        <fileset>
            <includes name="*.txt"/>
        </fileset>
    </copy>

    <!-- make sure the files got copied -->
    <fail unless="@{FileExists('sub/a.txt')}"/>
    <fail unless="@{FileExists('sub/b.txt')}"/>
    <fail unless="@{FileExists('sub/c.txt')}"/>
</project>

```


### Example ###
Copy a set of files in a different basedirectory to a new directory.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- create some test files -->
    <mkdir dir="source/foo"/>
    <mkdir dir="source/bar"/>
    <touch file="source/foo/a.txt"/>
    <touch file="source/bar/b.txt"/>
    <fail message="Source file not in expected location" unless="@{FileExists('source/foo/a.txt')}"/>
    
    <!-- copy the files to new directory (directory will get created) -->
    <copy todir="sub">
        <fileset basedir="source">
            <includes name="**/*.txt"/>
        </fileset>
    </copy>

    <!-- make sure the files got copied -->
    <fail message="Destination file not in expected location" unless="@{FileExists('sub/foo/a.txt')}"/>
    <fail message="Destination file not in expected location" unless="@{FileExists('sub/bar/b.txt')}"/>
</project>

```


### Example ###
Test to show how specifying duplicate source files doesn&#39;t cause problems.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo file="source.txt">A temp file.</echo>
    <copy todir="out">
        <fileset>
            <includes name="Source.txt"/>
            <includes name="Source.txt"/>
            <includes name="*/**.txt"/>
        </fileset>
    </copy>
    <fail message="File did not copy." unless="@{FileExists('out/source.txt')}"/>
</project>

```


### Example ###
Example of &lt;copy&gt; task with flatten set to true.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="srcDir" value="${nant.location}\..\config"/>
    <echo message="Copying NAnt's config folder to temp"/>
    <mkdir dir="temp"/>
    <copy todir="temp" flatten="true">
        <fileset basedir="${srcDir}">
            <includes name="**/*.*"/>
        </fileset>
    </copy>
    <echo>Folder temp has @{DirectoryGetFileCount('temp', '*.*')} files</echo>
    <echo>Deleting temp</echo>
    <delete dir="temp" failonerror="false"/>
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
<copy file="" tofile="" todir="" overwrite="" clobber="" maintainattributes="" flatten="" retrycount="" retrydelay="" hardlinkifpossible="" symlinkifpossible="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</copy>
```
