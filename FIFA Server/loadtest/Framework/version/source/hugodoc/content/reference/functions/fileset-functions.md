---
title: FileSet Functions
weight: 1008
---
## Summary ##
Collection of file set manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String FileSetExists(String filesetName)]({{< relref "#string-filesetexists-string-filesetname" >}}) | Check if the specified fileset is defined. |
| [String FileSetCount(String fileSetName)]({{< relref "#string-filesetcount-string-filesetname" >}}) | Gets the number of files contained in the FileSet. |
| [String FileSetGetItem(String fileSetName,Int32 index)]({{< relref "#string-filesetgetitem-string-filesetname-int32-index" >}}) | Returns the specified filename at the zero-based index of the specified FileSet. |
| [String FileSetToString(String fileSetName,String delimiter)]({{< relref "#string-filesettostring-string-filesetname-string-delimiter" >}}) | Converts a FileSet to a string. |
| [String FileSetUndefine(String filesetName)]({{< relref "#string-filesetundefine-string-filesetname" >}}) | Undefine an existing fileset |
| [String FileSetGetBaseDir(String fileSetName)]({{< relref "#string-filesetgetbasedir-string-filesetname" >}}) | Get the base directory of a fileset |
| [String FileSetDefinitionToXmlString(String fileSetName)]({{< relref "#string-filesetdefinitiontoxmlstring-string-filesetname" >}}) | Converts a FileSet to an XML string. |
## Detailed overview ##
#### String FileSetExists(String filesetName) ####
##### Summary #####
Check if the specified fileset is defined.

###### Parameter:  project ######


###### Parameter:  filesetName ######
The fileset name to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="f" />
    <fail unless="@{FileSetExists('f')}" />
</project>

```





#### String FileSetCount(String fileSetName) ####
##### Summary #####
Gets the number of files contained in the FileSet.

###### Parameter:  project ######


###### Parameter:  fileSetName ######
The name of the FileSet.

###### Return Value: ######
Number of file in the fileset.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="f">
        <includes name="one.txt" asis="true" />
        <includes name="two.txt" asis="true" />
    </fileset>
    
    <fail unless="@{FileSetCount('f')} == 2" />
</project>

```





#### String FileSetGetItem(String fileSetName,Int32 index) ####
##### Summary #####
Returns the specified filename at the zero-based index of the specified FileSet.

###### Parameter:  project ######


###### Parameter:  fileSetName ######
The name of the FileSet.

###### Parameter:  index ######
Zero-based index of the specified FileSet. Index must be non-negative and less than the size of the FileSet.

###### Return Value: ######
Filename at zero-based index of the FileSet. 

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="f">
        <includes name="one.txt" asis="true" />
        <includes name="two.txt" asis="true" />
    </fileset>
    
    <fail unless="'@{FileSetGetItem('f', '0')}' == 'one.txt'" />
    <fail unless="'@{FileSetGetItem('f', '1')}' == 'two.txt'" />
</project>

```





#### String FileSetToString(String fileSetName,String delimiter) ####
##### Summary #####
Converts a FileSet to a string.

###### Parameter:  project ######


###### Parameter:  fileSetName ######
The name of the FileSet.

###### Parameter:  delimiter ######
The delimiter used to separate each file.

###### Return Value: ######
A string of delimited files.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="f">
        <includes name="one.txt" asis="true" />
        <includes name="two.txt" asis="true" />
    </fileset>
    
    <fail unless="'@{FileSetToString('f', ';')}' == 'one.txt;two.txt'" />
</project>

```





#### String FileSetUndefine(String filesetName) ####
##### Summary #####
Undefine an existing fileset

###### Parameter:  project ######


###### Parameter:  filesetName ######
The fileset name to undefine.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="f" />
    <echo message="Fileset f exists: @{FileSetExists('f')}" />
    <echo message="Removing f: @{FileSetUndefine('f')}" />
    <fail message="f still exists" if="@{FileSetExists('f')} == True" />
</project>

```





#### String FileSetGetBaseDir(String fileSetName) ####
##### Summary #####
Get the base directory of a fileset

###### Parameter:  project ######


###### Parameter:  fileSetName ######


###### Return Value: ######
Path to the base directory

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fileset name="myfileset" basedir="${package.dir}/source" />
    <fail unless="@{FileSetGetBaseDir('myfileset')} eq '${package.dir}/source'" />
</project>

```





#### String FileSetDefinitionToXmlString(String fileSetName) ####
##### Summary #####
Converts a FileSet to an XML string.

###### Parameter:  project ######


###### Parameter:  fileSetName ######
The name of the FileSet.

###### Return Value: ######
string containing XML describing the fileset.




