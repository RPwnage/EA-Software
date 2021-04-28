---
title: < linker > Task
weight: 1048
---
## Syntax
```xml
<linker path="" version="" failonerror="" verbose="" if="" unless="">
  <system-librarydirs />
  <system-libs />
  <common-options />
  <template />
</linker>
```
## Summary ##
Setup c/c++ linker properties for used by the build optionsets.


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
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-librarydirs" >}}| Create readonly &quot;link.system-librarydirs&quot; property indicating system (SDK) library directories that will be applied to all build (optional).  Note that each path should be separated by new line. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "system-libs" >}}| Create readonly &quot;linker.system-libs&quot; property indicating system (SDK) libraries that will be applied to all builds (optional).  Note: Try not to use full path.  Just use library filename.  Each library should be separated by new line.<br>Library path information should be setup in &lt;system-librarydirs&gt; block. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "common-options" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.LinkerTemplates" "template" >}}|  | {{< task "EA.Eaconfig.Structured.LinkerTemplates" >}} | True |

## Full Syntax
```xml
<linker path="" version="" failonerror="" verbose="" if="" unless="">
  <system-librarydirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </system-librarydirs>
  <system-libs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </system-libs>
  <common-options if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </common-options>
  <template>
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
  </template>
</linker>
```
