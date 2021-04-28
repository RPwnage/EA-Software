---
title: SolutionFolder
weight: 1071
---
## Syntax
```xml
<SolutionFolder append="" name="">
  <items append="" name="" optionset="" />
  <projects />
  <folder append="" name="" />
  <echo />
  <do />
</SolutionFolder>
```
## Summary ##
Define folders for Visual Studio Solutions 


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| append | Append this definition to existing definition. Default: inherits from &#39;SolutionFolders&#39; element&#39; or parent folder | Boolean | False |
| name | The name of the folder. | String | True |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "items" >}}| Items (files) to put in this folder | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "projects" >}}| Packages and modules to include in the solution folders. Syntax is similar to dependency declarations. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.SolutionFolderCollection" "folder" >}}| Subfolders. | {{< task "EA.Eaconfig.Structured.SolutionFolderCollection" >}} | False |

## Full Syntax
```xml
<SolutionFolder append="" name="">
  <items append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </items>
  <projects if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </projects>
  <folder append="" name="" if="" unless="">
    <items />
    <projects />
    <folder />
  </folder>
  <echo />
  <do />
</SolutionFolder>
```
