---
title: < nant-coredump > Task
weight: 1090
---
## Syntax
```xml
<nant-coredump echo="" dump-properties="" dump-filesets="" dump-optionsets="" expand-all="" tofile="" format="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| echo | Write to the log file. Default is false | Boolean | False |
| dump-properties | Dump properties. Default - true. | Boolean | False |
| dump-filesets | Dump fileset names. Default - true. | Boolean | False |
| dump-optionsets | Dump optionset names | Boolean | False |
| expand-all | Include content of filesets and optionsets. Default is false | Boolean | False |
| tofile | Dump optionset names | String | False |
| format | format: text, xml. Default - text | Format | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<nant-coredump echo="" dump-properties="" dump-filesets="" dump-optionsets="" expand-all="" tofile="" format="" failonerror="" verbose="" if="" unless="" />
```
