---
title: < nativetoolchain > Task
weight: 1049
---
## Syntax
```xml
<nativetoolchain failonerror="" verbose="" if="" unless="">
  <compiler path-clanguage="" />
  <assembler />
  <linker />
  <librarian />
</nativetoolchain>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.CompilerTask" "compiler" >}}|  | {{< task "EA.Eaconfig.Structured.CompilerTask" >}} | True |
| {{< task "EA.Eaconfig.Structured.AssemblerTask" "assembler" >}}|  | {{< task "EA.Eaconfig.Structured.AssemblerTask" >}} | True |
| {{< task "EA.Eaconfig.Structured.LinkerTask" "linker" >}}|  | {{< task "EA.Eaconfig.Structured.LinkerTask" >}} | True |
| {{< task "EA.Eaconfig.Structured.LibrarianTask" "librarian" >}}|  | {{< task "EA.Eaconfig.Structured.LibrarianTask" >}} | True |

## Full Syntax
```xml
<nativetoolchain failonerror="" verbose="" if="" unless="">
  <compiler path-clanguage="" path="" version="" if="" unless="">
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
    <failonerror />
    <verbose />
  </compiler>
  <assembler path="" version="" if="" unless="">
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
    <failonerror />
    <verbose />
  </assembler>
  <linker path="" version="" if="" unless="">
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
    <failonerror />
    <verbose />
  </linker>
  <librarian path="" version="" if="" unless="">
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
    <failonerror />
    <verbose />
  </librarian>
</nativetoolchain>
```
