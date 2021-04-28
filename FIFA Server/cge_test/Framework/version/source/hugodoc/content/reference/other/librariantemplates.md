---
title: LibrarianTemplates
weight: 1045
---
## Syntax
```xml
<LibrarianTemplates>
  <objectfile />
  <commandline />
  <responsefile />
  <echo />
  <do />
</LibrarianTemplates>
```
## Summary ##
Setup c/c++ librarian (archiver) template information.  This block meant to be used under &lt;librarian&gt; block.


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "objectfile" >}}| Create template property lib.template.objectfile | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "commandline" >}}| Create template property {tool}.template.commandline.  Note that each argument should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" "responsefile" >}}| Templates that control response files, their contents and how they are passed to the build tool (optional) | {{< task "EA.Eaconfig.Structured.ResponseFileTemplates" >}} | False |

## Full Syntax
```xml
<LibrarianTemplates>
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
</LibrarianTemplates>
```
