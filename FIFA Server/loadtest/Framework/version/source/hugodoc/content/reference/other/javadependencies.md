---
title: JavaDependencies
weight: 1078
---
## Syntax
```xml
<JavaDependencies if="" unless="" value="" append="">
  <do />
</JavaDependencies>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |
| value | Argument. Default is null. | String | False |
| append | Append new data to the current value. The current value may come from partial modules. Default: &#39;true&#39;. | Boolean | False |

## Full Syntax
```xml
<JavaDependencies if="" unless="" value="" append="">
  <do />
</JavaDependencies>
```
