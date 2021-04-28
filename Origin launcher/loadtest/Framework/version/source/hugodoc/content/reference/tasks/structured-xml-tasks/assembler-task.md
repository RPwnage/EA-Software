---
title: < assembler > Task
weight: 1044
---
## Syntax
```xml
<assembler path="" version="" failonerror="" verbose="" if="" unless="">
  <common-defines />
  <system-includedirs />
  <common-options />
  <template />
</assembler>
```
## Summary ##
Setup c/c++ assembler properties for used by the build optionsets.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| path |  | String | True |
| version |  | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-defines" >}}| Create readonly &quot;as.common-defines&quot; property indicating defines that will be applied to all c++ assembly compiles (optional).  Note that each define should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-includedirs" >}}| Create readonly &quot;as.system-includedirs&quot; property indicating system (SDK) include directories that will be applied to all assembly compiles (optional).  Note that each path should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-options" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.AssemblerTemplates" "template" >}}|  | {{< task "EA.Eaconfig.Structured.AssemblerTemplates" >}} | True |

## Full Syntax
```xml
<assembler path="" version="" failonerror="" verbose="" if="" unless="">
  <common-defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-defines>
  <system-includedirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </system-includedirs>
  <common-options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-options>
  <template>
    <define if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </define>
    <includedir if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </includedir>
    <system-includedir if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </system-includedir>
    <commandline if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </commandline>
    <responsefile>
      <commandline if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </commandline>
      <contents if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </contents>
      <echo />
      <do />
    </responsefile>
    <echo />
    <do />
  </template>
</assembler>
```
