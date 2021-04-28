---
title: OptionSet Functions
weight: 1011
---
## Summary ##
Option set manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String OptionSetGetValue(String optionSetName,String optionName)]({{< relref "#string-optionsetgetvalue-string-optionsetname-string-optionname" >}}) | Get the value of an option in a named optionset. |
| [String OptionSetExists(String optionSetName)]({{< relref "#string-optionsetexists-string-optionsetname" >}}) | Check if the specified optionset is defined. |
| [String OptionSetOptionExists(String optionSetName,String optionName)]({{< relref "#string-optionsetoptionexists-string-optionsetname-string-optionname" >}}) | Check if the specified option is defined within the specified optionset. |
| [String OptionSetUndefine(String optionSetName)]({{< relref "#string-optionsetundefine-string-optionsetname" >}}) | Undefine the specified optionset. |
| [String OptionSetOptionUndefine(String optionSetName,String optionName)]({{< relref "#string-optionsetoptionundefine-string-optionsetname-string-optionname" >}}) | Undefine the specified optionset option. |
| [String OptionSetToString(String optionSetName)]({{< relref "#string-optionsettostring-string-optionsetname" >}}) | Output option set to string |
## Detailed overview ##
#### String OptionSetGetValue(String optionSetName,String optionName) ####
##### Summary #####
Get the value of an option in a named optionset.

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The name of the optionset to get from.

###### Parameter:  optionName ######
The name of the option to get.

###### Return Value: ######
The value of the option or an empty string if no option defined.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name='optset'>
        <option name='opt' value='1' />
    </optionset>
    
    <fail unless="@{OptionSetGetValue('optset', 'opt')} == 1" />
</project>

```





#### String OptionSetExists(String optionSetName) ####
##### Summary #####
Check if the specified optionset is defined.

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The name of the optionset.

###### Return Value: ######
True if the option set is defined, otherwise False.

returns&gt;###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name='optset' />

    <fail unless="@{OptionSetExists('optset')}" />
</project>

```





#### String OptionSetOptionExists(String optionSetName,String optionName) ####
##### Summary #####
Check if the specified option is defined within the specified optionset.

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The name of the optionset.

###### Parameter:  optionName ######
The name of the option.

###### Return Value: ######
True if the option is defined, otherwise False.

returns&gt;###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name='optset'>
        <option name='opt' value='1' />
    </optionset>
    
    <fail unless="@{OptionSetOptionExists('optset', 'opt')}" />
</project>

```





#### String OptionSetUndefine(String optionSetName) ####
##### Summary #####
Undefine the specified optionset.

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The optionset name to undefine.

###### Return Value: ######
True if the optionset was undefined properly, otherwise False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name='optset' />
    <eval code="@{OptionSetUndefine('optset')}" type="Function" />
    <fail if="@{OptionSetExists('optset')}"/>
</project>

```





#### String OptionSetOptionUndefine(String optionSetName,String optionName) ####
##### Summary #####
Undefine the specified optionset option.

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The name of the optionset.

###### Parameter:  optionName ######
The name of the option to undefine.

###### Return Value: ######
True if the optionset option was undefined properly, otherwise False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <optionset name='optset'>
        <option name='opt' value='1' />
    </optionset>
    
    <eval code="@{OptionSetOptionUndefine('optset', 'opt')}" type="Function" />
    
    <fail if="@{OptionSetOptionExists('optset', 'opt')}"/>
</project>

```





#### String OptionSetToString(String optionSetName) ####
##### Summary #####
Output option set to string

###### Parameter:  project ######


###### Parameter:  optionSetName ######
The optionset name.

###### Return Value: ######
string with optionset content.




