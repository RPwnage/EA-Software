---
title: < p4submit > Task
weight: 1154
---
## Syntax
```xml
<p4submit files="" change="" reopen="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Submit a changelist to the depot.

## Remarks ##
When a file has been opened by `p4add, p4edit, p4delete,`  or  `p4integrate` , the file is
added to a changelist.  The user&#39;s changes to the file are made only within in the client
workspace until the changelist is sent to the depot with `p4submit` .
            
If the `change` parameter is not specified, the default changelist will be used.
            
After task is executed the `change` parameter will contain actual submitted changelist.See thePerforce User&#39;s Guidefor more information.



### Example ###
The first command submits all the files from changelist `2112` , then reopens them for edit.The second submits all files that match  `//sandbox/*.txt` .

Perforce Commands:


```xml

p4 submit -r -c 2112
p4 submit //sandbox/*.txt
```
Equivalent NAnt Tasks:
```xml

<p4submit reopen="true" change="2112" />
<p4submit files="//sandbox/*.txt" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | A single file pattern may be specified as a parameter to a p4 submit of the<br>default changelist. This file pattern limits which files in the default changelist are<br>included in the submission; files that don&#39;t match the file pattern are moved to the<br>next default changelist.<br>The file pattern parameter to p4submit may not be used if `change` is specified. | String | False |
| change | Changelist to modify. After task is executed it contains actual submitted changelist | String | False |
| reopen | Reopens the submitted files for editing after submission. | Boolean | False |
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
<p4submit files="" change="" reopen="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
