---
title: File Functions
weight: 1007
---
## Summary ##
Collection of file manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String FileExists(String path)]({{< relref "#string-fileexists-string-path" >}}) | Determines whether the specified file exists. |
| [String FileCheckAttributes(String path,String attributes)]({{< relref "#string-filecheckattributes-string-path-string-attributes" >}}) | Check the FileAttributes of the file on the fully qualified path against a set of FileAttributes. |
| [String FileGetAttributes(String path)]({{< relref "#string-filegetattributes-string-path" >}}) | Gets the FileAttributes of the file on the fully qualified path. |
| [String FileGetLastAccessTime(String path)]({{< relref "#string-filegetlastaccesstime-string-path" >}}) | Gets the date time stamp the specified file or directory was last accessed. |
| [String FileGetLastWriteTime(String path)]({{< relref "#string-filegetlastwritetime-string-path" >}}) | Gets the date time stamp the specified file or directory was last written to. |
| [String FileGetCreationTime(String path)]({{< relref "#string-filegetcreationtime-string-path" >}}) | Gets the creation date time stamp of the specified file or directory. |
| [String FileGetVersion(String path)]({{< relref "#string-filegetversion-string-path" >}}) | Get the version number for the specified file. |
| [String FileGetSize(String path)]({{< relref "#string-filegetsize-string-path" >}}) | Get the size of the specified file. |
## Detailed overview ##
#### String FileExists(String path) ####
##### Summary #####
Determines whether the specified file exists.

###### Parameter:  project ######


###### Parameter:  path ######
The file to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <fail unless="@{FileExists('foo.txt')}" />
</project>

```





#### String FileCheckAttributes(String path,String attributes) ####
##### Summary #####
Check the FileAttributes of the file on the fully qualified path against a set of FileAttributes.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the file.

###### Parameter:  attributes ######
The set of attributes to check. Delimited by a space.

##### Remarks #####
&lt;dl&gt;
 &lt;dt&gt;List of available attributes.&lt;/dt&gt;
 &lt;dl&gt;
  &lt;li&gt; Archive &lt;/li&gt;
  &lt;li&gt; Compressed &lt;/li&gt;
  &lt;li&gt; Device &lt;/li&gt;
  &lt;li&gt; Directory &lt;/li&gt;
  &lt;li&gt; Encrypted &lt;/li&gt;
  &lt;li&gt; Hidden &lt;/li&gt;
  &lt;li&gt; Normal &lt;/li&gt;
  &lt;li&gt; NotContentIndexed &lt;/li&gt;
  &lt;li&gt; Offline &lt;/li&gt;
  &lt;li&gt; ReadOnly &lt;/li&gt;
  &lt;li&gt; ReparsePoint &lt;/li&gt;
  &lt;li&gt; SparseFile &lt;/li&gt;
  &lt;li&gt; System &lt;/li&gt;
  &lt;li&gt; Temporary &lt;/li&gt;
 &lt;/dl&gt;
&lt;/dl&gt;

###### Return Value: ######
True if all specified attributes are set; otherwise false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <attrib file='foo.txt' archive='true' />
    <fail unless="@{FileCheckAttributes('foo.txt', 'Archive')}" />
</project>

```





#### String FileGetAttributes(String path) ####
##### Summary #####
Gets the FileAttributes of the file on the fully qualified path.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the file.

##### Remarks #####
&lt;dl&gt;
 &lt;dt&gt;List of available attributes.&lt;/dt&gt;
 &lt;dl&gt;
  &lt;li&gt; Archive &lt;/li&gt;
  &lt;li&gt; Compressed &lt;/li&gt;
  &lt;li&gt; Device &lt;/li&gt;
  &lt;li&gt; Directory &lt;/li&gt;
  &lt;li&gt; Encrypted &lt;/li&gt;
  &lt;li&gt; Hidden &lt;/li&gt;
  &lt;li&gt; Normal &lt;/li&gt;
  &lt;li&gt; NotContentIndexed &lt;/li&gt;
  &lt;li&gt; Offline &lt;/li&gt;
  &lt;li&gt; ReadOnly &lt;/li&gt;
  &lt;li&gt; ReparsePoint &lt;/li&gt;
  &lt;li&gt; SparseFile &lt;/li&gt;
  &lt;li&gt; System &lt;/li&gt;
  &lt;li&gt; Temporary &lt;/li&gt;
 &lt;/dl&gt;
&lt;/dl&gt;

###### Return Value: ######
A set of FileAttributes separated by a space.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <attrib file='foo.txt' archive='true' />
    <fail unless="@{FileGetAttributes('foo.txt')} == Archive" />
</project>

```





#### String FileGetLastAccessTime(String path) ####
##### Summary #####
Gets the date time stamp the specified file or directory was last accessed.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the file or directory.

###### Return Value: ######
The date and time that the specified file or directory was last accessed.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <echo message="@{FileGetLastAccessTime('foo.txt')}" />
</project>

```





#### String FileGetLastWriteTime(String path) ####
##### Summary #####
Gets the date time stamp the specified file or directory was last written to.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the file or directory.

###### Return Value: ######
The date and time that the specified file or directory was last written to.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <echo message="@{FileGetLastWriteTime('foo.txt')}" />
</project>

```





#### String FileGetCreationTime(String path) ####
##### Summary #####
Gets the creation date time stamp of the specified file or directory.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the file or directory.

###### Return Value: ######
The date and time that the specified file or directory was created.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <echo message="@{FileGetCreationTime('foo.txt')}" />
</project>

```





#### String FileGetVersion(String path) ####
##### Summary #####
Get the version number for the specified file.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the specified file.

###### Return Value: ######
The version number for the specified file.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{FileGetVersion('${nant.location}\nant.exe')} == ${nant.version}" />
</project>

```





#### String FileGetSize(String path) ####
##### Summary #####
Get the size of the specified file.

###### Parameter:  project ######


###### Parameter:  path ######
The path to the specified file.

###### Return Value: ######
Size of the specified file in bytes.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="foo.txt" />
    <echo message="foo.txt is @{FileGetSize('foo.txt')} bytes"/>
</project>

```





