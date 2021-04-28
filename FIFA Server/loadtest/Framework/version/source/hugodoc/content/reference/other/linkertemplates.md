---
title: LinkerTemplates
weight: 1047
---
## Syntax
```xml
<LinkerTemplates>
  <librarydir />
  <libraryfile />
  <objectfile />
  <commandline />
  <responsefile />
  <echo />
  <do />
</LinkerTemplates>
```
## Summary ##
Setup c/c++ linker template information.  This block meant to be used under &lt;linker&gt; block.


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "librarydir" >}}| Create template property link.template.librarydir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "libraryfile" >}}| Create template property link.template.libraryfile | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "objectfile" >}}| Create template property link.template.objectfile | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "commandline" >}}| Create template property {tool}.template.commandline.  Note that each argument should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" "responsefile" >}}| Templates that control response files, their contents and how they are passed to the build tool (optional) | {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" >}} | False |

## Full Syntax
```xml
<LinkerTemplates>
  <librarydir if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </librarydir>
  <libraryfile if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </libraryfile>
  <objectfile if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </objectfile>
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
</LinkerTemplates>
```
