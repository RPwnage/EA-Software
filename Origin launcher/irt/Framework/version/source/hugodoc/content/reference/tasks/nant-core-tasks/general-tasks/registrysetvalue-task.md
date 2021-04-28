---
title: < RegistrySetValue > Task
weight: 1121
---
## Syntax
```xml
<RegistrySetValue hive="" key="" name="" value="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Add or update a value in an existing registry key.

## Remarks ##
This task will add or update a value in an existing registry key.
A build exception will be thrown if the key does not exist or the process
does not have sufficient privileges to modify the registry.  Use this
task with caution as changing registry values can wreak havoc on your
system.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| hive | The top-level node in the windows registry. Possible values are: LocalMachine, Users, CurrentUser, and ClassesRoot. | String | True |
| key | Key in which the value is to be updated. | String | True |
| name | Name of the value to add or change. | String | True |
| value | New value of the entry. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<RegistrySetValue hive="" key="" name="" value="" failonerror="" verbose="" if="" unless="" />
```
