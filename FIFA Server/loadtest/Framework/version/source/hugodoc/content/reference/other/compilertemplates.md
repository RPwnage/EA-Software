---
title: CompilerTemplates
weight: 1041
---
## Syntax
```xml
<CompilerTemplates>
  <commandline-clanguage />
  <pch.commandline />
  <define />
  <includedir />
  <system-includedir />
  <usingdir />
  <forceusing-assembly />
  <commandline />
  <responsefile />
  <echo />
  <do />
</CompilerTemplates>
```
## Summary ##
Setup c/c++ compiler template information.  This block meant to be used under &lt;compiler&gt; block.


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "commandline-clanguage" >}}| Create template property cc-clanguage.template.commandline (optional).  Note that each argument should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "pch.commandline" >}}| Create template property cc.template.pch.commandline (optional) | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "define" >}}| Create template property cc.template.define | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "includedir" >}}| Create template property cc.template.includedir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-includedir" >}}| Create template property cc.template.system-includedir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "usingdir" >}}| Create template property cc.template.usingdir (optional - should only get used for managed c++ build) | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "forceusing-assembly" >}}| Create template property cc.template.forceusing-assembly (optional - should only get used for managed c++ build) | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "commandline" >}}| Create template property {tool}.template.commandline.  Note that each argument should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" "responsefile" >}}| Templates that control response files, their contents and how they are passed to the build tool (optional) | {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" >}} | False |

## Full Syntax
```xml
<CompilerTemplates>
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
</CompilerTemplates>
```
