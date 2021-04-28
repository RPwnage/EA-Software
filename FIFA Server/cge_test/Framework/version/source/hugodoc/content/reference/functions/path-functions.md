---
title: Path Functions
weight: 1013
---
## Summary ##
Collection of path manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String PathGetFullPath(String path)]({{< relref "#string-pathgetfullpath-string-path" >}}) | Returns the complete path. |
| [String PathGetFileSystemFileName(String path)]({{< relref "#string-pathgetfilesystemfilename-string-path" >}}) | Return the true/real filename stored in the file system. If file doesn&#39;t exist, it will<br>just return the path passed in. |
| [String PathToUnix(String path)]({{< relref "#string-pathtounix-string-path" >}}) | Returns the path with backward slashes &#39;\&#39; converted to forward slashes &#39;/&#39;. |
| [String PathToCygwin(String path)]({{< relref "#string-pathtocygwin-string-path" >}}) | Returns the path with drive letter and semicolon are substituted to cygwin notation,<br>backward slashes &#39;\&#39; converted to forward slashes &#39;/&#39;. |
| [String PathToWindows(String path)]({{< relref "#string-pathtowindows-string-path" >}}) | Returns the string with forward slashes &#39;/&#39; converted to backward slashes &#39;\&#39;. |
| [String PathToNativeOS(String path)]({{< relref "#string-pathtonativeos-string-path" >}}) | Returns the string with the correct format for the current OS. |
| [String PathToWsl(String path)]({{< relref "#string-pathtowsl-string-path" >}}) | Returns the windows path but formated for the windows subsystem for linux.<br>Specifically drive letter, C:, is changed to /mnt/c, and slashes are reversed. |
| [String PathGetDirectoryName(String path)]({{< relref "#string-pathgetdirectoryname-string-path" >}}) | Returns the directory name for the specified path string. |
| [String PathGetRelativePath(String path,String basepath)]({{< relref "#string-pathgetrelativepath-string-path-string-basepath" >}}) | Relative path from the first given path to the second. |
| [String PathGetFileName(String path)]({{< relref "#string-pathgetfilename-string-path" >}}) | Returns the file name and extension of the specified path string. |
| [String PathGetExtension(String path)]({{< relref "#string-pathgetextension-string-path" >}}) | Returns the extension of the specified path string. |
| [String PathGetFileNameWithoutExtension(String path)]({{< relref "#string-pathgetfilenamewithoutextension-string-path" >}}) | Returns the file name of the specified path string without the extension. |
| [String PathChangeExtension(String path,String extension)]({{< relref "#string-pathchangeextension-string-path-string-extension" >}}) | Changes the extension of the specified path. |
| [String PathCombine(String path1,String path2)]({{< relref "#string-pathcombine-string-path1-string-path2" >}}) | Combine two specified paths. |
| [String PathGetPathRoot(String path)]({{< relref "#string-pathgetpathroot-string-path" >}}) | Returns the root directory of the specified path. |
| [String PathGetTempFileName()]({{< relref "#string-pathgettempfilename" >}}) | Returns a unique temporary file name which is created as an empty file on disk. |
| [String PathGetRandomFileName()]({{< relref "#string-pathgetrandomfilename" >}}) | Returns a cryptographically strong, random string that can be used as either a folder name or a file name. |
| [String PathGetTempPath()]({{< relref "#string-pathgettemppath" >}}) | Returns the path to the systems temp folder. |
| [String PathHasExtension(String path)]({{< relref "#string-pathhasextension-string-path" >}}) | Check if the specified path has a filename extension. |
| [String PathIsPathRooted(String path)]({{< relref "#string-pathispathrooted-string-path" >}}) | Check if the specified path contains absolute or relative path information. |
## Detailed overview ##
#### String PathGetFullPath(String path) ####
##### Summary #####
Returns the complete path.

###### Parameter:  project ######


###### Parameter:  path ######
The relative path to convert.

###### Return Value: ######
Complete path with drive letter (if applicable).

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathGetFullPath('.')}" />
</project>

```





#### String PathGetFileSystemFileName(String path) ####
##### Summary #####
Return the true/real filename stored in the file system. If file doesn&#39;t exist, it will
just return the path passed in.

###### Parameter:  project ######


###### Parameter:  path ######
full path or path that specify a file

###### Return Value: ######
if file exist, return the actual filename stored in the file system, else return the path passed in.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="FileName" value="@{PathGetTempPath()}Mixed Case.txt"/>
    <touch file="${FileName}"/>
    <property name="DOSPath" value="@{PathGetFileSystemFileName('@{PathGetTempPath()}MIXEDC~1.TXT')}"/>
    <fail
        unless="'@{PathGetFileName(${DOSPath})}' == '@{PathGetFileName(${FileName})}'"
        message="'@{PathGetFileName(${DOSPath})}' does not match case of '@{PathGetFileName(${FileName})}'."
        />
    <delete file="${FileName}"/>
</project>

```





#### String PathToUnix(String path) ####
##### Summary #####
Returns the path with backward slashes &#39;\&#39; converted to forward slashes &#39;/&#39;.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to convert.

###### Return Value: ######
Path with backward slashes converted to forward slashes.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathToUnix('c:\windows')}" />
</project>

```





#### String PathToCygwin(String path) ####
##### Summary #####
Returns the path with drive letter and semicolon are substituted to cygwin notation,
backward slashes &#39;\&#39; converted to forward slashes &#39;/&#39;.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to convert.

###### Return Value: ######
cygwin path.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathToUnix('c:\windows')}" />
</project>

```





#### String PathToWindows(String path) ####
##### Summary #####
Returns the string with forward slashes &#39;/&#39; converted to backward slashes &#39;\&#39;.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to convert.

###### Return Value: ######
Path with forward slashes converted to backward slashes.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathToWindows('/usr/linux')}" />
</project>

```





#### String PathToNativeOS(String path) ####
##### Summary #####
Returns the string with the correct format for the current OS.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to convert.

###### Return Value: ######
Path with correct format for the current OS.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathToUnix('c:\windows')}" />
</project>

```





#### String PathToWsl(String path) ####
##### Summary #####
Returns the windows path but formated for the windows subsystem for linux.
Specifically drive letter, C:, is changed to /mnt/c, and slashes are reversed.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to convert.

###### Return Value: ######
Path with correct format for WSL.




#### String PathGetDirectoryName(String path) ####
##### Summary #####
Returns the directory name for the specified path string.

###### Parameter:  project ######


###### Parameter:  path ######
The path of a file or directory.

###### Return Value: ######
Directory name of the specified path string.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathGetDirectoryName('packages\build')} == 'packages'" />
</project>

```





#### String PathGetRelativePath(String path,String basepath) ####
##### Summary #####
Relative path from the first given path to the second.

###### Parameter:  project ######


###### Parameter:  path ######
The path of a file or directory.

///###### Parameter:  basepath ######
Base path used to compute relative path.

###### Return Value: ######
Relative path or full path when computing relative path is not possible.




#### String PathGetFileName(String path) ####
##### Summary #####
Returns the file name and extension of the specified path string.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to obtain the file name and extension.

###### Return Value: ######
File name and extension of the specified path string.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathGetFileName('packages\nant.exe')} == 'nant.exe'" />
</project>

```





#### String PathGetExtension(String path) ####
##### Summary #####
Returns the extension of the specified path string.

###### Parameter:  project ######


###### Parameter:  path ######
The path string from which to get the extension.

###### Return Value: ######
Extension of the specified path string.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathGetExtension('nant.exe')} == '.exe'" />
</project>

```





#### String PathGetFileNameWithoutExtension(String path) ####
##### Summary #####
Returns the file name of the specified path string without the extension.

###### Parameter:  project ######


###### Parameter:  path ######
The path of the file.

###### Return Value: ######
File name of the specified path string without the extension.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathGetFileNameWithoutExtension('nant.exe')} == 'nant'" />
</project>

```





#### String PathChangeExtension(String path,String extension) ####
##### Summary #####
Changes the extension of the specified path.

###### Parameter:  project ######


###### Parameter:  path ######
The specified path.

###### Parameter:  extension ######
The new extension.

###### Return Value: ######
The specified path with a different extension.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathChangeExtension('nant.exe', 'cfg')} == 'nant.cfg'" />
</project>

```





#### String PathCombine(String path1,String path2) ####
##### Summary #####
Combine two specified paths.

###### Parameter:  project ######


###### Parameter:  path1 ######
The first specified path.

###### Parameter:  path2 ######
The second specified path.

###### Return Value: ######
A new path containing the combination of the two specified paths.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathCombine('packages', 'nant')} == 'packages\nant'" />
</project>

```





#### String PathGetPathRoot(String path) ####
##### Summary #####
Returns the root directory of the specified path.

###### Parameter:  project ######


###### Parameter:  path ######
The specified path.

###### Return Value: ######
The root directory of the specified path.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathGetPathRoot('d:\packages')}" />
</project>

```





#### String PathGetTempFileName() ####
##### Summary #####
Returns a unique temporary file name which is created as an empty file on disk.

###### Parameter:  project ######


###### Return Value: ######
The temporary file (full path) which has been created on disk at user&#39;s temporary folder.  Note that this function call will throw IOException if the temp file cannot be created!

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <trycatch>
        <try>
            <eval code="@{PathGetTempFileName()}" type="Function" property="temp-file"/>
        </try>
        <catch>
        </catch>
    </trycatch>
    <choose>
        <do if="@{FileExists(${temp-file})}">
            <echo message="${temp-file} is just created on disk." />
        </do>
        <do>
            <echo message="Unable to create ${temp-file} on disk."/>
        </do>
    </choose>
</project>

```





#### String PathGetRandomFileName() ####
##### Summary #####
Returns a cryptographically strong, random string that can be used as either a folder name or a file name.

###### Parameter:  project ######


###### Return Value: ######
A random string that can be used as either a folder name or a file name.  Unlike PathGetTempFileName(), this function will not create a file on disk.  This function is added in Framework 7.06.00.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathGetRandomFileName()}" />
</project>

```





#### String PathGetTempPath() ####
##### Summary #####
Returns the path to the systems temp folder.

###### Parameter:  project ######


###### Return Value: ######
The path to the systems temp folder.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{PathGetTempPath()}" />
</project>

```





#### String PathHasExtension(String path) ####
##### Summary #####
Check if the specified path has a filename extension.

###### Parameter:  project ######


###### Parameter:  path ######
The specified path.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathHasExtension('nant.exe')}" />
    <fail if="@{PathHasExtension('nant')}" />
</project>

```





#### String PathIsPathRooted(String path) ####
##### Summary #####
Check if the specified path contains absolute or relative path information.

###### Parameter:  project ######


###### Parameter:  path ######
The specified path.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{PathIsPathRooted('d:\nant.exe')}" />
    <fail if="@{PathIsPathRooted('nant.exe')}" />
</project>

```





