---
title: < p4change > Task
weight: 1144
---
## Syntax
```xml
<p4change desc="" files="" change="" delete="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Creates a new Perforce changelist or edits an existing one.

## Remarks ##
Files to be added to a changelist must already be opened for
&#39;add&#39; or for &#39;edit&#39; on the perforce server or this
command will fail.This task sets the  `${p4.change}` property which can be used with
the `P4Submit` ,  `P4Edit` , or  `P4Add` tasks.

If the `files` parameter is not specified, all files from the default changelist will be
moved to the new changelist.See thePerforce User&#39;s Guidefor more information.



### Example ###
Create a new changelist with the given description.Perforce Command:


```xml

p4 change "This is a new changelist created by NAnt"
```
Equivalent NAnt Task:
```xml

<p4change desc="This is a new changelist created by NAnt"/>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| desc | Textual description of the changelist. | String | False |
| files | A Perforce file specification for the files to add to the changelist. | String | False |
| change | Changelist to modify.  Required for delete. | String | False |
| delete | Delete the named changelist.  This only works for empty changelists. | Boolean | False |
| type | Set change list type to be restricted or public. This only works starting with P4 2012.1. | ChangeType | False |
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
<p4change desc="" files="" change="" delete="" type="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
