---
title: < p4delete > Task
weight: 1147
---
## Syntax
```xml
<p4delete files="" change="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Open file(s) in a client workspace for deletion from the depot.

## Remarks ##
This command marks files for deletion.
To actually delete the files, you must submit the changelist using the `p4submit` command.See thePerforce User&#39;s Guidefor more information.



### Example ###
Mark `//sandbox/Test.cs`  for delete on changelist  `12345` .Perforce Command:


```xml

p4 delete -c 12345 //sandbox/Test.cs
```
Equivalent NAnt Task:
```xml

<p4delete change="12345" files="//sandbox/Test.cs" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | Files to mark for delete. | String | True |
| change | Attach the given files to this changelist.<br>If not specified, the files get attached to the default changelist. | String | False |
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
<p4delete files="" change="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
