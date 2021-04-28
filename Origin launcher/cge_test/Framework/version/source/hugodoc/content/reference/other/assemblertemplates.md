---
title: AssemblerTemplates
weight: 1043
---
## Syntax
```xml
<AssemblerTemplates>
  <define />
  <includedir />
  <system-includedir />
  <commandline />
  <responsefile />
  <echo />
  <do />
</AssemblerTemplates>
```
## Summary ##
Setup c/c++ assembler template information.  This block meant to be used under &lt;assembler&gt; block.


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "define" >}}| Create template property as.template.define | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "includedir" >}}| Create template property as.template.includedir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-includedir" >}}| Create template property as.template.system-includedir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "commandline" >}}| Create template property {tool}.template.commandline.  Note that each argument should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" "responsefile" >}}| Templates that control response files, their contents and how they are passed to the build tool (optional) | {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" >}} | False |

## Full Syntax
```xml
<AssemblerTemplates>
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
</AssemblerTemplates>
```
