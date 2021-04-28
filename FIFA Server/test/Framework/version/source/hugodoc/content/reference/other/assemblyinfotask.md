---
title: AssemblyInfoTask
weight: 1004
---
## Syntax
```xml
<AssemblyInfoTask company="" copyright="" product="" title="" description="" version="" deterministic="">
  <echo />
  <do />
</AssemblyInfoTask>
```
## Summary ##
If this XML element is present on a module definition Framework will generate an AssemblyInfo file and add it to the project


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| company | Overrides the company field in the generated AssemblyInfo file, default is &quot;Electronic Arts&quot; | String | False |
| copyright | Overrides the copyright field in the generated AssemblyInfo file, default is &quot;(c) Electronic Arts. All Rights Reserved.&quot; | String | False |
| product | Overrides the product field in the generated AssemblyInfo file, default is the package&#39;s name | String | False |
| title | Overrides the title field in the generated AssemblyInfo file, default is &quot;(ModuleName).dll&quot; | String | False |
| description | Overrides the description field in the generated AssemblyInfo file, default is the an empty string. | String | False |
| version | Overrides the version field in the generated AssemblyInfo file, default is the package version | String | False |
| deterministic |  | Boolean | False |

## Full Syntax
```xml
<AssemblyInfoTask company="" copyright="" product="" title="" description="" version="" deterministic="">
  <echo />
  <do />
</AssemblyInfoTask>
```
