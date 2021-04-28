---
title: Directory Functions
weight: 1005
---
## Summary ##
Collection of directory manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String DirectoryExists(String path)]({{< relref "#string-directoryexists-string-path" >}}) | Determines whether the given path refers to an existing directory on disk. |
| [String DirectoryIsEmpty(String path)]({{< relref "#string-directoryisempty-string-path" >}}) | Returns true if the specified directory does not contain any files or directories; otherwise false. |
| [String DirectoryGetFileCount(String path,String pattern)]({{< relref "#string-directorygetfilecount-string-path-string-pattern" >}}) | Returns the number of files in a specified directory which match the given search pattern. |
| [String DirectoryGetFiles(String path,String pattern,Char delim)]({{< relref "#string-directorygetfiles-string-path-string-pattern-char-delim" >}}) | Returns a delimited string of file names in the given directory. |
| [String DirectoryGetDirectories(String path,String pattern,Char delim)]({{< relref "#string-directorygetdirectories-string-path-string-pattern-char-delim" >}}) | Returns a delimited string of subdirectory names in the given directory. |
| [String DirectoryGetDirectoriesRecursive(String path,String pattern,Char delim)]({{< relref "#string-directorygetdirectoriesrecursive-string-path-string-pattern-char-delim" >}}) | Returns a delimited string of recursively searched directory names from (and including) the provided path |
| [String DirectoryGetLastAccessTime(String path,String pattern)]({{< relref "#string-directorygetlastaccesstime-string-path-string-pattern" >}}) | Returns last accessed time of specified directory as a string. The string is formated using pattern specified by user. |
| [String DirectoryGetLastWriteTime(String path,String pattern)]({{< relref "#string-directorygetlastwritetime-string-path-string-pattern" >}}) | Returns last write time of specified directory as a string. The string is formated using pattern specified by user. |
| [String DirectoryMove(String source,String dest)]({{< relref "#string-directorymove-string-source-string-dest" >}}) | Moves the whole directory. When source and dest are on the same drive it just renames dir without copying files. |
## Detailed overview ##
#### String DirectoryExists(String path) ####
##### Summary #####
Determines whether the given path refers to an existing directory on disk.

###### Parameter:  project ######


###### Parameter:  path ######
The path to test.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <fail unless="@{DirectoryExists('foo')}" />
</project>

```





#### String DirectoryIsEmpty(String path) ####
##### Summary #####
Returns true if the specified directory does not contain any files or directories; otherwise false.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir='foo' />
    <fail unless="@{DirectoryIsEmpty('foo')}"
          message='Directory should have been empty' />
</project>

```





#### String DirectoryGetFileCount(String path,String pattern) ####
##### Summary #####
Returns the number of files in a specified directory which match the given search pattern.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
The search pattern to use.

###### Return Value: ######
Number of files in a specified directory that match the given search pattern.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <delete dir="foo" failonerror="false" />
    <mkdir dir="foo" />
    <touch file="foo/bar.txt" />
    
    <fail unless="@{DirectoryGetFileCount('foo', '*.txt')} == 1" />
</project>

```





#### String DirectoryGetFiles(String path,String pattern,Char delim) ####
##### Summary #####
Returns a delimited string of file names in the given directory.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
The search pattern to use.

###### Parameter:  delim ######
The delimiter to use.

###### Return Value: ######
Delimited string of file names.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <touch file="foo/bar.txt" />
    <touch file="foo/baz.txt" />
    
    <eval code="@{DirectoryGetFiles('foo', '*.*', '|')}"
          property="files"
          type="Function" />
    
    <foreach item="String" in="${files}" property="file" delim="|">
        <fail unless="@{FileExists('${file}')}" />
    </foreach>
</project>

```





#### String DirectoryGetDirectories(String path,String pattern,Char delim) ####
##### Summary #####
Returns a delimited string of subdirectory names in the given directory.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
The search pattern to use.

###### Parameter:  delim ######
The delimiter to use.

###### Return Value: ######
Delimited string of directory names.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <mkdir dir="foo/raz" />
    <mkdir dir="foo/bar" />
    
    <eval code="@{DirectoryGetDirectories('foo', '*', '|')}"
          property="directories"
          type="Function" />
    
    <foreach item="String" in="${directories}" property="directory" delim="|">
        <fail unless="@{DirectoryExists('${directory}')}" />
    </foreach>
</project>

```





#### String DirectoryGetDirectoriesRecursive(String path,String pattern,Char delim) ####
##### Summary #####
Returns a delimited string of recursively searched directory names from (and including) the provided path

###### Parameter:  project ######
The project being built

###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
The search pattern to use.

###### Parameter:  delim ######
The delimiter to use.

###### Return Value: ######
Delimited string of directory names: provided directory and children.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <mkdir dir="foo/raz" />
    <mkdir dir="foo/bar" />
    <mkdir dir="foo/bar/1" />
    <mkdir dir="foo/bar/2" />
    <mkdir dir="foo/bar/3" />

    <eval code="@{DirectoryGetDirectoriesRecursive('foo', '*', '|')}"
          property="directories"
          type="Function" />
    
    <foreach item="String" in="${directories}" property="directory" delim="|">
        <fail unless="@{DirectoryExists('${directory}')}" />
    </foreach>
</project>

```





#### String DirectoryGetLastAccessTime(String path,String pattern) ####
##### Summary #####
Returns last accessed time of specified directory as a string. The string is formated using pattern specified by user.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
format pattern.

###### Return Value: ######
Formated last access time of a directory specified by user.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <echo message="@{DirectoryGetLastAccessTime('foo', 'dd MMM yyyy')}"/>
    <echo message="@{DirectoryGetLastAccessTime('foo', 'yyyyMMdd')}"/>
    <echo message="@{DirectoryGetLastAccessTime('foo', 'HHmm')}"/>
</project>

```





#### String DirectoryGetLastWriteTime(String path,String pattern) ####
##### Summary #####
Returns last write time of specified directory as a string. The string is formated using pattern specified by user.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the directory.

###### Parameter:  pattern ######
format pattern.

###### Return Value: ######
Formated last access time of a directory specified by user.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <echo message="@{DirectoryGetLastWriteTime('foo', 'dd MMM yyyy')}"/>
    <echo message="@{DirectoryGetLastWriteTime('foo', 'yyyyMMdd')}"/>
    <echo message="@{DirectoryGetLastWriteTime('foo', 'HHmm')}"/>
</project>

```





#### String DirectoryMove(String source,String dest) ####
##### Summary #####
Moves the whole directory. When source and dest are on the same drive it just renames dir without copying files.

###### Parameter:  project ######


###### Parameter:  source ######
The path to the source directory.

###### Parameter:  dest ######
The path to destination directory.

###### Return Value: ######
Full path to the destination directory.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <mkdir dir="foo" />
    <fail unless="@{DirectoryMove('foo', 'bar')}" />
</project>

```





