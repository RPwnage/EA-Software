---
title: PrecompiledHeadersElement
weight: 1026
---
## Syntax
```xml
<PrecompiledHeadersElement enable="" pchfile="" pchheader="" pchheaderdir="" use-forced-include="" if="" unless="" />
```
## Summary ##
Precompiled headers input.   NOTE: To use this option, you still need to list a source file and specify that file with &#39;create-precompiled-header&#39; optionset.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| enable | enable/disable using precompiled headers. | Boolean | False |
| pchfile | Name of output precompiled header (Note that some platform&#39;s VSI don&#39;t provide this setting.  So this value may not get used! | String | False |
| pchheader | Name of the precompiled header file | String | False |
| pchheaderdir | Directory to the precompiled header file | String | False |
| use-forced-include | Use forced include command line switches to set up include to the header file so that people don&#39;t need to modify<br>all source files to do #include &quot;pch.h&quot; | Boolean | False |
| if | If true then the target will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the target will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<PrecompiledHeadersElement enable="" pchfile="" pchheader="" pchheaderdir="" use-forced-include="" if="" unless="" />
```
