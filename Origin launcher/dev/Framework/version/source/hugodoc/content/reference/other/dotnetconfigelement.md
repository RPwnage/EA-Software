---
title: DotNetConfigElement
weight: 1073
---
## Syntax
```xml
<DotNetConfigElement>
  <buildoptions />
  <defines />
  <additionaloptions />
  <copylocal />
  <platform />
  <allowunsafe />
  <targetframeworkversion />
  <languageversion />
  <usewpf />
  <usewindowsforms />
  <targetframeworkfamily />
  <generateserializationassemblies />
  <suppresswarnings />
  <warningsaserrors.list />
  <warningsaserrors />
  <warningsnotaserrors.list />
  <remove />
  <preprocess />
  <postprocess />
  <echo />
  <do />
</DotNetConfigElement>
```
### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.BuildTypeElement" "buildoptions" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTypeElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "defines" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "additionaloptions" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "copylocal" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "platform" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "allowunsafe" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "targetframeworkversion" >}}|  | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "languageversion" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "usewpf" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "usewindowsforms" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "targetframeworkfamily" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "generateserializationassemblies" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "suppresswarnings" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "warningsaserrors.list" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "warningsaserrors" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "warningsnotaserrors.list" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DotNetModuleTask+RemoveDotNetBuildOptions" "remove" >}}|  | {{< task "EA.Eaconfig.Structured.DotNetModuleTask+RemoveDotNetBuildOptions" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "preprocess" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "postprocess" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<DotNetConfigElement>
  <buildoptions if="" unless="" fromoptionset="">
    <option />
  </buildoptions>
  <defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </defines>
  <additionaloptions if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </additionaloptions>
  <copylocal if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </copylocal>
  <platform if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </platform>
  <allowunsafe if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </allowunsafe>
  <targetframeworkversion />
  <languageversion if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </languageversion>
  <usewpf if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </usewpf>
  <usewindowsforms if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </usewindowsforms>
  <targetframeworkfamily if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </targetframeworkfamily>
  <generateserializationassemblies if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </generateserializationassemblies>
  <suppresswarnings if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </suppresswarnings>
  <warningsaserrors.list if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </warningsaserrors.list>
  <warningsaserrors if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </warningsaserrors>
  <warningsnotaserrors.list if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </warningsnotaserrors.list>
  <remove>
    <defines if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </defines>
    <options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </options>
    <echo />
    <do />
  </remove>
  <preprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </preprocess>
  <postprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </postprocess>
  <echo />
  <do />
</DotNetConfigElement>
```
