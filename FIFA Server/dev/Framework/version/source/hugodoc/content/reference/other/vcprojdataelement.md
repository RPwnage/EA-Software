---
title: VcprojDataElement
weight: 1035
---
## Syntax
```xml
<VcprojDataElement>
  <pre-link-step />
  <input-resource-manifests append="" name="" optionset="" />
  <additional-manifest-files append="" name="" optionset="" />
  <echo />
  <do />
</VcprojDataElement>
```
## Summary ##



### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "pre-link-step" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "input-resource-manifests" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "additional-manifest-files" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |

## Full Syntax
```xml
<VcprojDataElement>
  <pre-link-step if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </pre-link-step>
  <input-resource-manifests append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </input-resource-manifests>
  <additional-manifest-files append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </additional-manifest-files>
  <echo />
  <do />
</VcprojDataElement>
```
