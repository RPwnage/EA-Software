---
title: RemoveBuildOptions
weight: 1028
---
## Syntax
```xml
<RemoveBuildOptions>
  <defines />
  <cc.options />
  <as.options />
  <link.options />
  <lib.options />
  <echo />
  <do />
</RemoveBuildOptions>
```
## Summary ##
Sets options to be removed from final configuration


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "defines" >}}| Defines to be removed | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "cc.options" >}}| C/CPP compiler options to be removed | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "as.options" >}}| Assembly compiler options to be removed | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "link.options" >}}| Linker options to be removed | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "lib.options" >}}| librarian options to be removed | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<RemoveBuildOptions>
  <defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </defines>
  <cc.options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </cc.options>
  <as.options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </as.options>
  <link.options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </link.options>
  <lib.options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </lib.options>
  <echo />
  <do />
</RemoveBuildOptions>
```
