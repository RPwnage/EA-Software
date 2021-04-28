---
title: < nant > Task
weight: 1112
---
## Syntax
```xml
<nant buildfile="" buildpackage="" target="" optionset="" indent="" global-properties-action="" start-new-build="" failonerror="" verbose="" if="" unless="">
  <property />
  <out-property />
</nant>
```
## Summary ##
Runs NAnt on a supplied build file.

## Remarks ##
This task can be used to build subprojects which have their own full build files.  See the for a good example on how to build sub projects only once per build.




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| buildfile | The name of the *.build file to use. | String | False |
| buildpackage | The name of the package from which to use .build file. | String | False |
| target | The target to execute.  To specify more than one target separate targets with a space.  Targets are executed in order if possible.  Default to use target specified in the project&#39;s default attribute. | String | False |
| optionset | The name of an optionset containing a set of properties to be passed into the supplied build file. | String | False |
| indent | The log IndentLevel. Default is the current log IndentLevel + 1. | Int32 | False |
| global-properties-action | Defines how global properties are set up in the dependent project.<br>Valid values are**propagate** and **initialize**. Default is &#39;propagate&#39;. | GlobalPropertiesActionTypes | False |
| start-new-build | Start new build graph in the dependent project. Default is &#39;false&#39; | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<nant buildfile="" buildpackage="" target="" optionset="" indent="" global-properties-action="" start-new-build="" failonerror="" verbose="" if="" unless="">
  <property />
  <out-property />
</nant>
```
