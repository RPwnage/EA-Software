---
title: < GenerateBulkBuildFiles > Task
weight: 1227
---
## Syntax
```xml
<GenerateBulkBuildFiles FileSetName="" OutputFilename="" OutputDir="" MaxSize="" pch-header="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="" failonerror="" verbose="" if="" unless="">
  <output-bulkbuild-files defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <output-loose-files defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</GenerateBulkBuildFiles>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| FileSetName |  | String | True |
| OutputFilename |  | String | True |
| OutputDir |  | String | False |
| MaxSize |  | String | False |
| pch-header |  | String | False |
| SplitByDirectories |  | Boolean | False |
| MinSplitFiles |  | String | False |
| DeviateMaxSizeAllowance |  | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "output-bulkbuild-files" >}}|  | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "output-loose-files" >}}|  | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<GenerateBulkBuildFiles FileSetName="" OutputFilename="" OutputDir="" MaxSize="" pch-header="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="" failonerror="" verbose="" if="" unless="">
  <output-bulkbuild-files defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </output-bulkbuild-files>
  <output-loose-files defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </output-loose-files>
</GenerateBulkBuildFiles>
```
