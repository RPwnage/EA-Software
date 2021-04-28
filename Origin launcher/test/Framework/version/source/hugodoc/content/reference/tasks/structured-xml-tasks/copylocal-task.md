---
title: < copylocal > Task
weight: 1021
---
## Syntax
```xml
<copylocal use-hardlink-if-possible="" if="" unless="" value="" append="">
  <do />
</copylocal>
```
## Summary ##
Module&#39;s global copylocal setting.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| use-hardlink-if-possible | Specifies that the copylocal task will use hard link if possible (default is false) | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |
| value | Argument. Default is null. | String | False |
| append | Append new data to the current value. The current value may come from partial modules. Default: &#39;true&#39;. | Boolean | False |

## Full Syntax
```xml
<copylocal use-hardlink-if-possible="" if="" unless="" value="" append="">
  <do />
</copylocal>
```
