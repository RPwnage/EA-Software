---
title: RunDataElement
weight: 1030
---
## Syntax
```xml
<RunDataElement workingdir="" args="" startupprogram="">
  <workingdir />
  <args />
  <startupprogram />
  <echo />
  <do />
</RunDataElement>
```
## Summary ##



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| workingdir | Sets the current working directory from which the executable needs to run | String | False |
| args | Sets the command line arguments for an executable | String | False |
| startupprogram | Sets the startup program | String | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "workingdir" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "args" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "startupprogram" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<RunDataElement workingdir="" args="" startupprogram="">
  <workingdir if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </workingdir>
  <args if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </args>
  <startupprogram if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </startupprogram>
  <echo />
  <do />
</RunDataElement>
```
