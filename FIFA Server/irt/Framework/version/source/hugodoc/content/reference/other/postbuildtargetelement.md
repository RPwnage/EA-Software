---
title: PostBuildTargetElement
weight: 1007
---
## Syntax
```xml
<PostBuildTargetElement skip-if-up-to-date="" targetname="" nant-build-only="">
  <target depends="" description="" hidden="" style="" name="" />
  <command />
  <echo />
  <do />
</PostBuildTargetElement>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| skip-if-up-to-date |  | Boolean | False |
| targetname | Sets the target name | String | False |
| nant-build-only | Autoconvert target to command when needed in case command is not defined. Default is &#39;true&#39; | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.TargetElement" "target" >}}| Sets the target | {{< task "EA.Eaconfig.Structured.TargetElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "command" >}}| Sets the comand | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<PostBuildTargetElement skip-if-up-to-date="" targetname="" nant-build-only="">
  <target depends="" description="" hidden="" style="" name="" if="" unless="">
    <!--NAnt Script-->
  </target>
  <command if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </command>
  <echo />
  <do />
</PostBuildTargetElement>
```
