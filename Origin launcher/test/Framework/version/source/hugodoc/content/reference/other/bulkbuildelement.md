---
title: BulkBuildElement
weight: 1012
---
## Syntax
```xml
<BulkBuildElement enable="" partial="" maxsize="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="">
  <loosefiles append="" name="" optionset="" />
  <sourcefiles name="" />
  <manualsourcefiles append="" name="" optionset="" />
  <echo />
  <do />
</BulkBuildElement>
```
## Summary ##
Bulkbuild input


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| enable | enable/disable bulkbuild generation. | Boolean | False |
| partial | If partial enabled, source files with custom build settings are excluded from bulkbuild generation and compiled separately. | Boolean | False |
| maxsize | If &#39;maxsize&#39; is set, generated bulkbuild files will contain no more than maxsize entries. I.e. They are split in several if number of files exceeds maxsize. | Int32 | False |
| SplitByDirectories | If &#39;SplitByDirectories&#39; is set, generated bulkbuild files will be split according to directories (default is false). | Boolean | False |
| MinSplitFiles | Use &#39;MinSplitFiles&#39; to specify minimum files when directory is split into a separate BulkBuild file (only used when SplitByDirectories is turned on).  That is, if current directory has less then the specified minimun files, this directory&#39;s files will be merged with the next directory to form a group and then do a split in this group. | Int32 | False |
| DeviateMaxSizeAllowance | Use &#39;DeviateMaxSizeAllowance&#39; to specify a threadhold of number of files we&#39;re allowed to deviate from maxsize input.  This parameter<br>only get used on incremental build where existing bulkbuild files have already been created from previous build.  Purpose of this parameter<br>is to allow your development process to not get slowed down by adding/removing files.  Default is set to 5. | Int32 | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "loosefiles" >}}| Files that are not included in the bulkbuild | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.NamedStructuredFileSets" "sourcefiles" >}}| Groups of sourcefiles to be used to generate bulkbuild files | {{< task "EA.Eaconfig.Structured.NamedStructuredFileSets" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "manualsourcefiles" >}}| Manual bulkbuild files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |

## Full Syntax
```xml
<BulkBuildElement enable="" partial="" maxsize="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="">
  <loosefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </loosefiles>
  <sourcefiles name="" append="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <name />
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </sourcefiles>
  <manualsourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </manualsourcefiles>
  <echo />
  <do />
</BulkBuildElement>
```
