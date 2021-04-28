---
title: Property Functions
weight: 1016
---
## Summary ##
Collection of property manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String PropertyExists(String propertyName)]({{< relref "#string-propertyexists-string-propertyname" >}}) | Check if the specified property is defined. |
| [String PropertyTrue(String propertyName)]({{< relref "#string-propertytrue-string-propertyname" >}}) | Check if the specified property value is true.<br>If property does not exist, an Exception will be thrown. |
| [String PropertyEqualsAny(String propertyName,String comparands)]({{< relref "#string-propertyequalsany-string-propertyname-string-comparands" >}}) | Check if the specified property value equals the argument |
| [String PropertyExpand(String propertyName)]({{< relref "#string-propertyexpand-string-propertyname" >}}) | Expand the specified property. |
| [String PropertyUndefine(String propertyName)]({{< relref "#string-propertyundefine-string-propertyname" >}}) | Undefine the specified property. |
| [String IsPropertyGlobal(String propertyName)]({{< relref "#string-ispropertyglobal-string-propertyname" >}}) | Check if the specified property is defined as global property in masterconfig. |
| [String GetPropertyOrDefault(String propertyName,String defaultVal)]({{< relref "#string-getpropertyordefault-string-propertyname-string-defaultval" >}}) | Returns property value or default value if property does not exist. |
## Detailed overview ##
#### String PropertyExists(String propertyName) ####
##### Summary #####
Check if the specified property is defined.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="p" value="1" />
    <fail unless="@{PropertyExists('p')}" />
</project>

```





#### String PropertyTrue(String propertyName) ####
##### Summary #####
Check if the specified property value is true.
If property does not exist, an Exception will be thrown.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="p" value="true" />
    <fail unless="@{PropertyTrue('p')}" />
</project>

```





#### String PropertyEqualsAny(String propertyName,String comparands) ####
##### Summary #####
Check if the specified property value equals the argument

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to check.

###### Parameter:  comparands ######
A comma-seperated-list of string values to compare against. They are trimmed.

###### Return Value: ######
True or False.




#### String PropertyExpand(String propertyName) ####
##### Summary #####
Expand the specified property.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to expand.

###### Return Value: ######
The value of the specified property. If the property does not exits a BuildException is thrown.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="p" value="1" />
    <fail unless="@{PropertyExpand('p')} == 1" />
</project>

```





#### String PropertyUndefine(String propertyName) ####
##### Summary #####
Undefine the specified property.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to undefine.

###### Return Value: ######
True if the property was undefined properly, otherwise False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="p" value="1" />

    <eval code="@{PropertyUndefine('p')}" type='Function' />
    <fail if="@{PropertyExists('p')}" />
</project>

```





#### String IsPropertyGlobal(String propertyName) ####
##### Summary #####
Check if the specified property is defined as global property in masterconfig.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="p" value="1" />
    <fail unless="@{IsPropertyGlobal('p')}" />
</project>

```





#### String GetPropertyOrDefault(String propertyName,String defaultVal) ####
##### Summary #####
Returns property value or default value if property does not exist.

###### Parameter:  project ######


###### Parameter:  propertyName ######
The property name to check.

###### Parameter:  defaultVal ######
default value.

###### Return Value: ######
property value.




