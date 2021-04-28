---
title: < declare-public-properties > Task
weight: 1053
---
## Syntax
```xml
<declare-public-properties packagename="" failonerror="" verbose="" if="" unless="">
  <property name="" description="" />
</declare-public-properties>
```
## Summary ##
Public property declaration allows to describe properties thatcan affect/control package build


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | Name of the package this property affects. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.DeclarePublicPropertiesTask+PublicPropertyDeclaration" "property" >}}| Public property declaration allows to describe properties | {{< task "EA.Eaconfig.Structured.DeclarePublicPropertiesTask+PublicPropertyDeclaration" >}} | False |

## Full Syntax
```xml
<declare-public-properties packagename="" failonerror="" verbose="" if="" unless="">
  <property name="" description="" />
</declare-public-properties>
```
