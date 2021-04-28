---
title: ResourceFilesElement
weight: 1029
---
## Syntax
```xml
<ResourceFilesElement prefix="" resourcebasedir="" includedirs="" defines="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="">
  <includes />
  <excludes />
  <do />
  <group />
</ResourceFilesElement>
```
## Summary ##



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| prefix | Indicates the prefix to prepend to the actual resource.  This is usually the<br>default namespace of the assembly. | String | False |
| resourcebasedir |  | String | False |
| includedirs | Additional include directories to pass to the resource compiler | String | False |
| defines | Additional defines to pass to the resource compiler | String | False |
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
<ResourceFilesElement prefix="" resourcebasedir="" includedirs="" defines="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
  <includes />
  <excludes />
  <do />
  <group />
</ResourceFilesElement>
```
