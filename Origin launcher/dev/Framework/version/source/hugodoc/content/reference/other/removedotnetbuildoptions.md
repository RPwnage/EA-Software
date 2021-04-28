---
title: RemoveDotNetBuildOptions
weight: 1074
---
## Syntax
```xml
<RemoveDotNetBuildOptions>
  <defines />
  <options />
  <echo />
  <do />
</RemoveDotNetBuildOptions>
```
### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "defines" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "options" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<RemoveDotNetBuildOptions>
  <defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </defines>
  <options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </options>
  <echo />
  <do />
</RemoveDotNetBuildOptions>
```
