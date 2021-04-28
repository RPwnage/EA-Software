---
title: < p4labelsync > Task
weight: 1153
---
## Syntax
```xml
<p4labelsync label="" files="" delete="" add="" execute="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
This task causes the named label to reflect the current contents of the client workspace.

## Remarks ##
A label records the last revision of each file it contains and can subsequently be used in
a revision specification as `@label` .If the files parameter is omitted, the sync will cause the label to reflect the entire
contents of the client by adding, deleting and updating the list of files in the label.
If the file parameter is included, the sync updates only the named files.

NOTE: You can only update labels you own, and labels that are locked cannot be updated
with `p4labelsync` .

See thePerforce User&#39;s Guidefor more information.



### Example ###
The first command lists what would happen if it were to add the files in `//sandbox/...`  to the label  `labelName` .The second deletes files matching  `//sandbox/*.txt`  from label  `labelName2` .

Perforce Commands:


```xml

p4 labelsync -a -n -l labelName //sandbox/...
p4 labelsync -d -l labelName2 //sandbox/*.txt
```
Equivalent NAnt Tasks:
```xml

<p4labelsync  label="labelName" files="//sandbox/..." add="true" execute="false" />
<p4labelsync  label="labelName2" files="//sandbox/*.txt" delete="true"  />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| label | Name to use for the label (required). | String | True |
| files | Optional Perforce file spec to limit the operation to the given files. | String | False |
| delete | Deletes the named files from the label.  Defaults false. | Boolean | False |
| add | Adds the named files to the label without deleting any files,<br>even if some of the files have been deleted at the head revision.  Defaults false. | Boolean | False |
| execute | If false, displays the results of this sync without actually executing it.<br>Defaults true. | Boolean | False |
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
<p4labelsync label="" files="" delete="" add="" execute="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
