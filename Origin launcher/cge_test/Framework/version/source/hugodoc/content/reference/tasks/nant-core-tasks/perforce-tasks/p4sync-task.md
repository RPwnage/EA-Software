---
title: < p4sync > Task
weight: 1157
---
## Syntax
```xml
<p4sync files="" force="" execute="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Synchronize files between the depot and the workspace.

## Remarks ##
See thePerforce User&#39;s Guidefor more information.



### Example ###
The first command brings the entire workspace into sync with the depot.The second command merely lists the files that are out of sync with the label  `labelname` .

Perforce Commands:


```xml

p4 sync
p4 sync -f -n @labelname.
```
Equivalent NAnt Tasks:
```xml

<p4sync />
<p4sync files="@labelname" force="true" execute="false" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | The files to sync. | String | False |
| force | Perform the sync even if files are not writable or are already in sync. | Boolean | False |
| execute | If false, displays the results of this sync without actually executing it (defaults true). | Boolean | False |
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
<p4sync files="" force="" execute="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
