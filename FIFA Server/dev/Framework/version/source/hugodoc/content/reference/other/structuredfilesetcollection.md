---
title: StructuredFileSetCollection
weight: 1032
---
## Syntax
```xml
<StructuredFileSetCollection if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
  <includes />
  <excludes />
  <do />
  <group />
</StructuredFileSetCollection>
```
## Summary ##
Sometimes it&#39;s useful to be able to access each fileset defined as a collection rather than
appending from multiple definitions into a single fileset. Gives the facade of a fileset so
the XML declaration can be the same (minus properties which don&#39;t make sense for structured.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |
| basedir |  | String | False |
| fromfileset |  | String | False |
| failonmissing |  | Boolean | False |
| name |  | String | False |
| append |  | Boolean | False |

## Full Syntax
```xml
<StructuredFileSetCollection if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
  <includes />
  <excludes />
  <do />
  <group />
</StructuredFileSetCollection>
```
