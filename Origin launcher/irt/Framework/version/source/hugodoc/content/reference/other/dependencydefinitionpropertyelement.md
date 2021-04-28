---
title: DependencyDefinitionPropertyElement
weight: 1017
---
## Syntax
```xml
<DependencyDefinitionPropertyElement interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
  <do />
</DependencyDefinitionPropertyElement>
```
## Summary ##



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| interface | Public include directories from dependent packages are added. | Boolean | False |
| link | Public libraries from dependent packages are added if this attribute is true. | Boolean | False |
| copylocal | Set copy local flag for this dependency output. | Boolean | False |
| internal | Deprecated, this will be ignored. | Nullable`1 | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |
| value | Argument. Default is null. | String | False |
| append | Append new data to the current value. The current value may come from partial modules. Default: &#39;true&#39;. | Boolean | False |

## Full Syntax
```xml
<DependencyDefinitionPropertyElement interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
  <do />
</DependencyDefinitionPropertyElement>
```
