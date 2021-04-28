---
title: UtilityConfigElement
weight: 1079
---
## Syntax
```xml
<UtilityConfigElement>
  <preprocess />
  <postprocess />
  <echo />
  <do />
</UtilityConfigElement>
```
### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "preprocess" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "postprocess" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<UtilityConfigElement>
  <preprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </preprocess>
  <postprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </postprocess>
  <echo />
  <do />
</UtilityConfigElement>
```
