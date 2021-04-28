---
title: < p4edit > Task
weight: 1150
---
## Syntax
```xml
<p4edit files="" change="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Open file(s) for edit in the local workspace.

## Remarks ##
The specified files will be linked to the a changelist, but changes to the depot will not
occur until a `p4submit` has been completed.If the  `change`  parameter is not specified, the files will go onto the  `default` changelist.
If the files were already open for edit and the `change` parameter is given, the files will be moved from their current changelist to the specified changelist.

See thePerforce User&#39;s Guidefor more information.



### Example ###
Open the files in `//depot/test/...`  for edit and place them on changelist  `12345` .Perforce Command:


```xml

p4 edit -c 12345 //depot/test/...
```
Equivalent NAnt Task:
```xml

<p4edit change="12345" files="//depot/test/..."/>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | The files to edit. | String | True |
| change | Assign these files to this pre-existing changelist. | String | False |
| type | Overrides the default type of the files to this type. | String | False |
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
<p4edit files="" change="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
