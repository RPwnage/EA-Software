---
title: < p4where > Task
weight: 1156
---
## Syntax
```xml
<p4where filespec="" property="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Return the file name in the local view of the client given the name in the depot.

## Remarks ##
NOTE: If verbose is true and the local file (or directory) doesn&#39;t exist,
a warning will be printed.See thePerforce User&#39;s Guidefor more information.



### Example ###
Print the local file name which is identified in the depot as `//depot/orca/bob.txt` .Perforce Command (the third line in the output):


```xml

p4 -ztag where //depot/orca/bob.txt
```
Equivalent NAnt Task:
```xml

<p4where filespec="//depot/orca/bob.txt" property="p4.where"/>
<echo message="Local path is: ${p4.where}"/>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| filespec | Perforce file spec in depot syntax. | String | True |
| property | The property to set with the required path. | String | True |
| type | The type of path required Can be one of these: Local, Depot or Client.. | String | False |
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
<p4where filespec="" property="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
