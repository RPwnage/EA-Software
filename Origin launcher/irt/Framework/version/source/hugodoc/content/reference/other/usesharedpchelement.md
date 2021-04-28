---
title: UseSharedPchElement
weight: 1027
---
## Syntax
```xml
<UseSharedPchElement module="" use-shared-binary="" if="" unless="" />
```
## Summary ##
Shared Precompiled Header module input.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| module | The shared pch module (use the form [package_name]/[build_group]/[module_name]).  The build_group field can be ignored if it is runtime. | String | True |
| use-shared-binary | For platforms that support using common shared pch binary, this attribute allow module level control of using the shared pch binary. (default = true) | Boolean | False |
| if | If true then the target will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the target will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<UseSharedPchElement module="" use-shared-binary="" if="" unless="" />
```
