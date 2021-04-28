---
title: < SharedPch > Task
weight: 1066
---
## Syntax
```xml
<SharedPch name="">
  <echo />
  <do />
</SharedPch>
```
## Summary ##
A special task to define a shared precompiled header module name in build file.  Note that to
use this task, people must first declare this module in Initialize.xml using &#39;sharedpchmodule&#39; public data task.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of this shared precompiled header module. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<SharedPch name="" failonerror="" verbose="" if="" unless="">
  <echo />
  <do />
</SharedPch>
```
