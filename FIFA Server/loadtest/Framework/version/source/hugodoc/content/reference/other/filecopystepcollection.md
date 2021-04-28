---
title: FileCopyStepCollection
weight: 1080
---
## Syntax
```xml
<FileCopyStepCollection file="" tofile="" todir="" if="" unless="">
  <file />
  <tofile />
  <todir />
  <fileset />
</FileCopyStepCollection>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| file |  | String | False |
| tofile |  | String | False |
| todir |  | String | False |
| if | If true then the target will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the target will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<FileCopyStepCollection file="" tofile="" todir="" if="" unless="">
  <file />
  <tofile />
  <todir />
  <fileset />
</FileCopyStepCollection>
```
