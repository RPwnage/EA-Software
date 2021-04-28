---
title: < p4revert > Task
weight: 1155
---
## Syntax
```xml
<p4revert files="" change="" onlyUnchanged="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Discard changes made to open files.

## Remarks ##
 `revert` causes perforce to discard all changes including checked-in changes made after the revision to which the reverted files are returned.  These changes are lost, and cannot be recovered in any way.

See thePerforce User&#39;s Guidefor more information.



### Example ###
Revert all unchanged files from changelist `12345`  that match the filter  `//depot/test/...` .Perforce Command:


```xml

p4 revert -a -c 12345 //depot/test/...
```
Equivalent NAnt Task:
```xml

<p4revert onlyUnchanged="true" change="12345" files="//depot/test/..." />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | The files to revert. | String | True |
| change | Revert only the files in the specified changelist. | String | False |
| onlyUnchanged | Revert only unchanged files (open for edit, but not altered). | Boolean | False |
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
<p4revert files="" change="" onlyUnchanged="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
