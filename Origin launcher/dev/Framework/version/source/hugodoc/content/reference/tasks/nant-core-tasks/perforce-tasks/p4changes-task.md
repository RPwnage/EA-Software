---
title: < p4changes > Task
weight: 1145
---
## Syntax
```xml
<p4changes files="" integrated="" maxnum="" status="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
This task lists submitted and pending changelists.

## Remarks ##
If no parameters are given, this task
reports the list of all pending and submitted changelists currently known to the perforce system.
The parameters serve to filter the output down to a manageable level.See thePerforce User&#39;s Guidefor more information.



### Example ###
List the last 3 submitted changelists in `//sandbox/...` Perforce Command:


```xml

p4 changes -m 3 //sandbox/...
```
Equivalent NAnt Task:
```xml

<p4changes maxnum=3 files="//sandbox/..." />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | Optional Perforce file spec to limit the output to the given files. | String | False |
| integrated | Include changelists that were integrated with the specified files. | Boolean | False |
| maxnum | Lists only the highest maxnum changes. | Int32 | False |
| status | Limit the list to the changelists with the given status (pending or submitted). | P4Status | False |
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
<p4changes files="" integrated="" maxnum="" status="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
