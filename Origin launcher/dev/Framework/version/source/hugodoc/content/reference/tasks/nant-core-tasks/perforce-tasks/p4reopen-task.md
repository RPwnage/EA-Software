---
title: < p4reopen > Task
weight: 1152
---
## Syntax
```xml
<p4reopen files="" change="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Moves opened files between changelists.

## Remarks ##
See thePerforce User&#39;s Guidefor more information.



### Example ###
Move the file `//sandbox/testFile.cs`  from the default changelist to changelist  `605040` .Perforce Command:


```xml

p4 reopen -c 605040 //sandbox/testFile.cs
```
Equivalent NAnt Task:
```xml

<p4reopen change="605040" files="//sandbox/testFile.cs" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | The files to operate on. | String | True |
| change | Revert only the files in the specified changelist. | String | True |
| quiet | Suppresses normal output messages but still prints error messages. | Boolean | False |
| client | Overrides P4CLIENT with this client name.<br>Can also be set via the ${p4.client} property. | String | False |
| dir | Overrides PWD with the specified directory.<br>Can also be set via the ${p4.pwd} property. | String | False |
| host | Overrides P4HOST with specified hostname.<br>Can also be set via the ${p4.host} property. | String | False |
| port | Overrides P4PORT with specified hostname.<br>Can also be set via the ${p4.port} property. | String | False |
| password | Overrides P4PASSWD with specified hostname.<br>Can also be set via the ${p4.passwd} property. | String | False |
| user | Overrides P4USER|USER|USERNAME with specified hostname.<br>Can also be set via the ${p4.user} property. | String | False |
| charset | Overrides P4CHARSET with specified charset.<br>Can also be set via the ${p4.charset} property. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<p4reopen files="" change="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
