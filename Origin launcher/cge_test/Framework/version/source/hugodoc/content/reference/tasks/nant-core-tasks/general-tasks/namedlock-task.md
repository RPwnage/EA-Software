---
title: < namedlock > Task
weight: 1113
---
## Syntax
```xml
<namedlock name="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Allows wrapping of a group of tasks to be executed in a mutually exclusive way based on &#39;name&#39; attribute.

## Remarks ##
This task acts as a named mutex for a group of nested tasks.

Make sure that name name value does not collide with namedlock names that can be defined in other packages.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | Unique name for the lock. All &lt;namedlock&gt; sections with same name are mutually exclusive<br>Make sure that key string does not collide with names of &lt;namedlock&gt; that may be defined in other packages.<br>Using &quot;package.[package name].&quot; prefix is a good way to ensure unique values. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<namedlock name="" failonerror="" verbose="" if="" unless="" />
```
