---
title: Registry Functions
weight: 1019
---
## Summary ##
Collection of windows registry manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String RegistryGetValue(RegistryHive hive,String key,String value)]({{< relref "#string-registrygetvalue-registryhive-hive-string-key-string-value" >}}) | Get the specified value of the specified key in the windows registry. |
| [String RegistryGetValue(RegistryHive hive,String key)]({{< relref "#string-registrygetvalue-registryhive-hive-string-key" >}}) | Get the specified value of the specified key in the windows registry. |
| [String RegistryKeyExists(RegistryHive hive,String key)]({{< relref "#string-registrykeyexists-registryhive-hive-string-key" >}}) | Checks that the specified key exists in the windows registry. |
| [String RegistryValueExists(RegistryHive hive,String key,String value)]({{< relref "#string-registryvalueexists-registryhive-hive-string-key-string-value" >}}) | Checks that the specified value of the specified key exists in the windows registry. |
| [String RegistryValueExists(RegistryHive hive,String key)]({{< relref "#string-registryvalueexists-registryhive-hive-string-key" >}}) | Checks that the specified value of the specified key exists in the windows registry. |
## Detailed overview ##
#### String RegistryGetValue(RegistryHive hive,String key,String value) ####
##### Summary #####
Get the specified value of the specified key in the windows registry.

###### Parameter:  project ######


###### Parameter:  hive ######
The top-level node in the windows registry. Possible values are: LocalMachine, Users,
CurrentUser, and ClassesRoot.

###### Parameter:  key ######
The name of the windows registry key.

###### Parameter:  value ######
The name of the windows registry key value.

###### Return Value: ######
The specified value of the specified key in the windows registry.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Framework -->
    <echo message="@{RegistryGetValue('LocalMachine', 'SOFTWARE\Electronic Arts\Framework', 'Path')}" />
</project>

```





#### String RegistryGetValue(RegistryHive hive,String key) ####
##### Summary #####
Get the specified value of the specified key in the windows registry.

###### Parameter:  project ######


###### Parameter:  hive ######
The top-level node in the windows registry. Possible values are: LocalMachine, Users,
CurrentUser, and ClassesRoot.

###### Parameter:  key ######
The name of the windows registry key.

###### Parameter:  value ######
The name of the windows registry key value.

###### Return Value: ######
The specified value of the specified key in the windows registry.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Framework -->
    <echo message="@{RegistryGetValue('LocalMachine', 'SOFTWARE\Electronic Arts\Framework', 'Path')}" />
</project>

```





#### String RegistryKeyExists(RegistryHive hive,String key) ####
##### Summary #####
Checks that the specified key exists in the windows registry.

###### Parameter:  project ######


###### Parameter:  hive ######
The top-level node in the windows registry. Possible values are: LocalMachine, Users,
CurrentUser, and ClassesRoot.

###### Parameter:  key ######
The name of the windows registry key.

###### Return Value: ######
True if the specified registry key exists, otherwise false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Framework -->
    <echo message="@{RegistryKeyExists('LocalMachine', 'SOFTWARE\Electronic Arts\Framework')}" />
</project>

```





#### String RegistryValueExists(RegistryHive hive,String key,String value) ####
##### Summary #####
Checks that the specified value of the specified key exists in the windows registry.

###### Parameter:  project ######


###### Parameter:  hive ######
The top-level node in the windows registry. Possible values are: LocalMachine, Users,
CurrentUser, and ClassesRoot.

###### Parameter:  key ######
The name of the windows registry key.

###### Parameter:  value ######
The name of the windows registry key value.

###### Return Value: ######
True if the value exists, otherwise false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Framework -->
    <echo message="@{RegistryValueExists('LocalMachine', 'SOFTWARE\Electronic Arts\Framework', 'Path')}" />
</project>

```





#### String RegistryValueExists(RegistryHive hive,String key) ####
##### Summary #####
Checks that the specified value of the specified key exists in the windows registry.

###### Parameter:  project ######


###### Parameter:  hive ######
The top-level node in the windows registry. Possible values are: LocalMachine, Users,
CurrentUser, and ClassesRoot.

###### Parameter:  key ######
The name of the windows registry key.

###### Parameter:  value ######
The name of the windows registry key value.

###### Return Value: ######
True if the value exists, otherwise false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Framework -->
    <echo message="@{RegistryValueExists('LocalMachine', 'SOFTWARE\Electronic Arts\Framework', 'Path')}" />
</project>

```





