---
title: < SolutionFolders > Task
weight: 1068
---
## Syntax
```xml
<SolutionFolders append="">
  <folder append="" name="" />
  <echo />
  <do />
</SolutionFolders>
```
## Summary ##
Define folders for Visual Studio Solutions 


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| append | Append this definition to existing definition. Default: &#39;true&#39;. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.SolutionFolderCollection" "folder" >}}| Folders. | {{< task "EA.Eaconfig.Structured.SolutionFolderCollection" >}} | False |

## Full Syntax
```xml
<SolutionFolders append="" failonerror="" verbose="" if="" unless="">
  <folder append="" name="" if="" unless="">
    <items />
    <projects />
    <folder />
  </folder>
  <echo />
  <do />
</SolutionFolders>
```
