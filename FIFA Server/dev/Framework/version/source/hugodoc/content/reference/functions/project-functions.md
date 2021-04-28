---
title: Project Functions
weight: 1015
---
## Summary ##
Collection of NAnt Project routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String ProjectGetLastError()]({{< relref "#string-projectgetlasterror" >}}) | Returns the previous build exception message. |
| [String ProjectGetLastInnerError()]({{< relref "#string-projectgetlastinnererror" >}}) | Returns the previous inner build exception message. |
| [String ProjectGetName()]({{< relref "#string-projectgetname" >}}) | Gets the name of the project. |
| [String ProjectGetDefaultTarget()]({{< relref "#string-projectgetdefaulttarget" >}}) | Gets the name of the default target. |
## Detailed overview ##
#### String ProjectGetLastError() ####
##### Summary #####
Returns the previous build exception message.

###### Parameter:  project ######


###### Return Value: ######
The previous build exception message that was thrown.

##### Remarks #####
If no error message exists an empty string is returned.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{ProjectGetLastError()}" />
</project>

```





#### String ProjectGetLastInnerError() ####
##### Summary #####
Returns the previous inner build exception message.

###### Parameter:  project ######


###### Return Value: ######
The previous inner build exception message that was thrown.

##### Remarks #####
If no error message exists an empty string is returned.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{ProjectGetLastInnerError()}" />
</project>

```





#### String ProjectGetName() ####
##### Summary #####
Gets the name of the project.

###### Parameter:  project ######


###### Return Value: ######
The project name or empty string if none exists.

###### Example ######

```xml

<project name='test'>
    <fail unless="'@{ProjectGetName()}' == 'test'" />
</project>

```





#### String ProjectGetDefaultTarget() ####
##### Summary #####
Gets the name of the default target.

###### Parameter:  project ######


###### Return Value: ######
The name of the default target or empty string if none exists.

###### Example ######

```xml

<project default='build'>
    <target name='build'>
        <fail unless="'@{ProjectGetDefaultTarget()}' == 'build'" />
    </target>
</project>

```





