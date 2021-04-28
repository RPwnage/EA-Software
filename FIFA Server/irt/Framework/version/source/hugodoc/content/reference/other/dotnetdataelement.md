---
title: DotNetDataElement
weight: 1019
---
## Syntax
```xml
<DotNetDataElement>
  <copylocal />
  <allowunsafe />
  <webreferences append="" />
  <echo />
  <do />
</DotNetDataElement>
```
## Summary ##
MCPP/DotNet specific data


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "copylocal" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "allowunsafe" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredOptionSet" "webreferences" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredOptionSet" >}} | False |

## Full Syntax
```xml
<DotNetDataElement>
  <copylocal if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </copylocal>
  <allowunsafe if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </allowunsafe>
  <webreferences append="" fromoptionset="">
    <option />
    <!--Structured Optionset-->
  </webreferences>
  <echo />
  <do />
</DotNetDataElement>
```
