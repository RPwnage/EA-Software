---
title: DotNetAssembliesFileset
weight: 1018
---
## Syntax
```xml
<DotNetAssembliesFileset usedefaultassemblies="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="">
  <includes />
  <excludes />
  <do />
  <group />
</DotNetAssembliesFileset>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| usedefaultassemblies | Controls whether default assemblies dependencies should be included. | Boolean | False |
| append | If true, the patterns specified by this task are added to the patterns contained in the fileset. If false, the fileset will only contains the patterns specified in this task. Default is &quot;true&quot;. | Boolean | False |
| name |  | String | False |
| optionset |  | String | False |
| defaultexcludes | Indicates whether default excludes should be used or not.  Default &quot;false&quot;. | Boolean | False |
| basedir | The base of the directory of this file set.  Default is project base directory. | String | False |
| failonmissing | Indicates if a build error should be raised if an explicitly included file does not exist.  Default is true. | Boolean | False |
| fromfileset | The name of a file set to include. | String | False |
| sort | Sort the fileset by filename. Default is false. | Boolean | False |
| if | If true then the target will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the target will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<DotNetAssembliesFileset usedefaultassemblies="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
  <includes />
  <excludes />
  <do />
  <group />
</DotNetAssembliesFileset>
```
