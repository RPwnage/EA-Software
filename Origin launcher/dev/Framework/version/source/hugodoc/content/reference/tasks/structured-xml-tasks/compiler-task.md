---
title: < compiler > Task
weight: 1042
---
## Syntax
```xml
<compiler path-clanguage="" path="" version="" failonerror="" verbose="" if="" unless="">
  <common-defines />
  <system-includedirs />
  <common-defines-clanguage />
  <common-options-clanguage />
  <common-options />
  <template />
</compiler>
```
## Summary ##
Setup c/c++ compiler properties for used by the build optionsets.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| path-clanguage | Create readonly &quot;cc-clanguage&quot; property (or verify property is not changed) and points to full path of c compiler (optional). | String | False |
| path |  | String | True |
| version |  | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-defines" >}}| Create readonly &quot;cc.common-defines&quot; property indicating defines that will be applied to all c++ compiles (optional).  Note that each define should be separated by new line.( | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-includedirs" >}}| Create readonly &quot;cc.system-includedirs&quot; property indicating system (SDK) include directories that will be applied to all c/c++ compiles (optional).  Note that each path should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-defines-clanguage" >}}| Create readonly &quot;cc-clanguage.common-defines&quot; property indicating defines that will be applied to all c compiles (optional).  Note that each define should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-options-clanguage" >}}| Create readonly &quot;cc-clanguage.common-options&quot; property indicating compiler options that will be applied to all c compiles (optional).  Note that each option should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-options" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.CompilerTemplates" "template" >}}|  | {{< task "EA.Eaconfig.Structured.CompilerTemplates" >}} | True |

## Full Syntax
```xml
<compiler path-clanguage="" path="" version="" failonerror="" verbose="" if="" unless="">
  <common-defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-defines>
  <system-includedirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </system-includedirs>
  <common-defines-clanguage if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-defines-clanguage>
  <common-options-clanguage if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-options-clanguage>
  <common-options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-options>
  <template>
    <commandline-clanguage if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </commandline-clanguage>
    <pch.commandline if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </pch.commandline>
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
    <usingdir if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </usingdir>
    <forceusing-assembly if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </forceusing-assembly>
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
</compiler>
```
