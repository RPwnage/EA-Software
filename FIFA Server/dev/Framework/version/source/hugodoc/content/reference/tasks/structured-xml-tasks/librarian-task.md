---
title: < librarian > Task
weight: 1046
---
## Syntax
```xml
<librarian path="" version="" failonerror="" verbose="" if="" unless="">
  <common-options />
  <template />
</librarian>
```
## Summary ##
Setup c/c++ librarian (archiver) properties for used by the build optionsets.


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
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-options" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.LibrarianTemplates" "template" >}}|  | {{< task "EA.Eaconfig.Structured.LibrarianTemplates" >}} | True |

## Full Syntax
```xml
<librarian path="" version="" failonerror="" verbose="" if="" unless="">
  <common-options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-options>
  <template>
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
  </template>
</librarian>
```
