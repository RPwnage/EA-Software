---
title: < p4add > Task
weight: 1145
---
## Syntax
```xml
<p4add files="" type="" change="" ignore="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Open file(s) from the local workspace for addition to the depot.

## Remarks ##
See thePerforce User&#39;s Guidefor more information.

### Example ###
Add all the files found in `//depot/test/` and below as `binary`  files and place them on changelist  `10101` .Perforce Command:


```xml

p4 add -t binary -c 10101 //depot/test/...
```
Equivalent NAnt Task:
```xml

<p4add type="binary" change="10101" files="//depot/test/text.txt" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| files | A Perforce file specification for the files to add. | String | True |
| type | Overrides the default type of the files to this type.<br>Legal values: &#39;text&#39;, &#39;binary&#39;, &#39;symlink&#39;, &#39;apple&#39;, &#39;resource&#39;, &#39;unicode&#39;. | String | False |
| change | Assign these files to this pre-existing changelist.<br>If not specified, files will belong to the &#39;default&#39; changelist. | String | False |
| ignore | The ignore attribute informs the client that it should not perform any<br>ignore checking configured by P4IGNORE.  Default value is false. | Boolean | False |
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
<p4add files="" type="" change="" ignore="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
