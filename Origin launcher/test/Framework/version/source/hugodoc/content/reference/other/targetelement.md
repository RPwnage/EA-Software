---
title: TargetElement
weight: 1008
---
## Syntax
```xml
<TargetElement depends="" description="" hidden="" style="" name="" if="" unless="" />
```
## Summary ##
Create a dynamic target. This task is a Framework 2 feature


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| depends | A space separated list of target names that this target depends on. | String | False |
| description | The Target description. | String | False |
| hidden | Prevents the target from being listed in the project help. Default is true. | Boolean | False |
| style | Style can be &#39;use&#39;, &#39;build&#39;, or &#39;clean&#39;.   See &#39;Auto Build Clean&#39;<br>page in the Reference/NAnt/Fundamentals section of the help doc for details. | String | False |
| name | This attribute is not used. Adding it here only to prevent build breaks after turning on strict attribute check. | String | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<TargetElement depends="" description="" hidden="" style="" name="" if="" unless="" />
```
