---
title: < p4label > Task
weight: 1152
---
## Syntax
```xml
<p4label label="" description="" lock="" files="" delete="" force="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Creates a new label and associates that label with the current revisions of a list of files on the Perforce depot.

## Remarks ##
Note: This command only creates a label and associates files with it.  Use `p4labelsync` to make the label permanent.See thePerforce User&#39;s Guidefor more information.



### Example ###
Create a label named `Nightly build` , with description  `Built on ${TIMESTAMP}` .
Lock the label once created, and associate the files in `//sandbox/...` with the label.Perforce Command:


```xml

p4 label -o "Nightly build"
[edit the form put out by the previous command to include the description, lock and file list data].
p4 label -i
p4 labelsync
```
Equivalent NAnt Task:
```xml

<p4label label="Nightly build" files="//sandbox/..." desc="Built on ${TIMESTAMP}" lock="true"/>
<p4labelsync />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| label | Name to use for the label (required). | String | True |
| description | Description of the label. | String | False |
| lock | Lock the label once created. | Boolean | False |
| files | Optional Perforce file spec to limit the output to the given files. | String | False |
| delete | Deletes an existing label. | Boolean | False |
| force | For delete, force delete even if locked.  Ignored otherwise. | Boolean | False |
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
<p4label label="" description="" lock="" files="" delete="" force="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
