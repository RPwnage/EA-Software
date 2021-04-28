---
title: < p4counter > Task
weight: 1148
---
## Syntax
```xml
<p4counter counter="" value="" property="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" />
```
## Summary ##
Obtain or set the value of a Perforce counter.

## Remarks ##
NOTE: The user performing this task must have Perforce &#39;review&#39; permission in order for this
task to succeed.See thePerforce User&#39;s Guidefor more information.



### Example ###
The first command prints the value of the counter `last-clean-build` .The second sets the value to  `999` .

The third sets the NAnt property value  `${p4.LastCleanBuild}` based on the value retrieved
from Perforce.

Perforce Commands:


```xml

p4 counter last-clean-build
p4 counter last-clean-build 999
[no real equivalent command]
```
Equivalent NAnt Tasks:
```xml

<p4counter counter="last-clean-build" />
<p4counter counter="last-clean-build" value="${TSTAMP}" />
<p4counter counter="last-clean-build" property="p4.LastCleanBuild" />
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| counter | The name of the counter. | String | True |
| value | The new value for the counter. | String | False |
| property | The property to set with the value of the counter. | String | False |
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
<p4counter counter="" value="" property="" quiet="" client="" dir="" host="" port="" password="" user="" charset="" failonerror="" verbose="" if="" unless="" />
```
